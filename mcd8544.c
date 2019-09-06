/*
 * MicroPython Nokia 5110 PCD8544 84x48 LCD driver
 * https://github.com/mcauser/micropython-pcd8544-c
 *
 * MIT License
 * Copyright (c) 2019 Mike Causer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define __MCD8544_VERSION__  "0.0.1"

#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"

#include "py/mphal.h"
#include "extmod/machine_spi.h"
#include "ports/stm32/font_petme128_8x8.h"

#include "mcd8544.h"

#define DC_LOW()     (mp_hal_pin_write(self->dc, 0))
#define DC_HIGH()    (mp_hal_pin_write(self->dc, 1))
#define CS_LOW()     { if(self->cs) {mp_hal_pin_write(self->cs, 0);} }
#define CS_HIGH()    { if(self->cs) {mp_hal_pin_write(self->cs, 1);} }
#define RESET_LOW()  { if(self->reset) {mp_hal_pin_write(self->reset, 0);} }
#define RESET_HIGH() { if(self->reset) {mp_hal_pin_write(self->reset, 1);} }

STATIC void write_spi(mp_obj_base_t *spi_obj, const uint8_t *buf, int len) {
    mp_machine_spi_p_t *spi_p = (mp_machine_spi_p_t*)spi_obj->type->protocol;
    spi_p->transfer(spi_obj, len, buf, NULL);
}

typedef struct _mcd8544_MCD8544_obj_t {
    mp_obj_base_t base;

    mp_obj_base_t *spi_obj;
    mp_hal_pin_obj_t dc;
    mp_hal_pin_obj_t cs;
    mp_hal_pin_obj_t reset;
    uint8_t fn;
} mcd8544_MCD8544_obj_t;

mp_obj_t mcd8544_MCD8544_make_new( const mp_obj_type_t *type,
                                  size_t n_args,
                                  size_t n_kw,
                                  const mp_obj_t *args );

STATIC void mcd8544_MCD8544_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<MCD8544 spi=%p>", self->spi_obj);
}


STATIC void write_cmd(mcd8544_MCD8544_obj_t *self, uint8_t cmd) {
    CS_LOW()
    DC_LOW();
    write_spi(self->spi_obj, &cmd, 1);
    CS_HIGH()
}

STATIC void write_data(mcd8544_MCD8544_obj_t *self, const uint8_t *data, int len) {
    CS_LOW()
    DC_HIGH();
    if (len > 0) {
        write_spi(self->spi_obj, data, len);
    }
    CS_HIGH()
}


STATIC void write_extended_cmd(mcd8544_MCD8544_obj_t *self, uint8_t vop, uint8_t bias, uint8_t temp) {
    // switch to extended instruction set
    // extended instruction set is required to set temp, bias and vop
    write_cmd(self, self->fn | MCD8544_EXTENDED_INSTR);
    // set temperature coefficient
    write_cmd(self, MCD8544_TEMP_COEFF | temp); // 0x04 | 0..3
    // set bias system (n=3 recommended mux rate 1:40/1:34)
    write_cmd(self, MCD8544_BIAS | bias); // 0x10 | 0..7
    // set contrast with operating voltage (0x00~0x7f)
    // 0x00 = 3.00V, 0x3f = 6.84V, 0x7f = 10.68V
    // starting at 3.06V, each bit increments voltage by 0.06V at room temperature
    write_cmd(self, MCD8544_VOP | vop);
    // revert to basic instruction set
    write_cmd(self, self->fn & ~MCD8544_EXTENDED_INSTR);
}


STATIC mp_obj_t mcd8544_MCD8544_reset(mp_obj_t self_in) {
    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // reset pulse soft resets the display
    // you need to call power() or init() to resume
    RESET_HIGH();
    mp_hal_delay_us(100);
    RESET_LOW();
    mp_hal_delay_us(100); // reset pulse has to be >100 ns and <100 ms
    RESET_HIGH();
    mp_hal_delay_us(100);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mcd8544_MCD8544_reset_obj, mcd8544_MCD8544_reset);


STATIC void mcd8544_MCD8544_init_internal(mcd8544_MCD8544_obj_t *self, bool horizontal, uint8_t vop, uint8_t bias, uint8_t temp) {
    // reset pulse
    mcd8544_MCD8544_reset(self);

    // Horizontal is the default. Switch to vertical?
    //if (!mp_obj_new_bool(args[ARG_horizontal].u_bool)) {
    if (!horizontal) {
        self->fn |= MCD8544_ADDRESSING_VERT;
    }

    // power on
    self->fn &= ~MCD8544_POWER_DOWN;

    // Set voltages
    // uint8_t vop = args[ARG_vop].u_int;
    // uint8_t bias = args[ARG_bias].u_int;
    // uint8_t temp = args[ARG_temp].u_int;

    // execute both extended and basic instruction set
    write_extended_cmd(self, vop, bias, temp);

    // display on
    write_cmd(self, MCD8544_DISPLAY_NORMAL);
}

STATIC mp_obj_t mcd8544_MCD8544_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_horizontal, ARG_vop, ARG_bias, ARG_temp };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_horizontal, MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_vop,        MP_ARG_INT, {.u_int = MCD8544_VOP_DEFAULT} },
        { MP_QSTR_bias,       MP_ARG_INT, {.u_int = MCD8544_BIAS_DEFAULT} },
        { MP_QSTR_temp,       MP_ARG_INT, {.u_int = MCD8544_TEMP_COEFF_DEFAULT} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    mcd8544_MCD8544_init_internal(
        self,
        args[ARG_horizontal].u_bool,
        args[ARG_vop].u_int,
        args[ARG_bias].u_int,
        args[ARG_temp].u_int
    );
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mcd8544_MCD8544_init_obj, 0, mcd8544_MCD8544_init);


STATIC mp_obj_t mcd8544_MCD8544_power(size_t n_args, const mp_obj_t *args) {
    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1 || mp_obj_is_true(args[1])) {
        self->fn &= ~MCD8544_POWER_DOWN;
    } else {
        self->fn |= MCD8544_POWER_DOWN;
    }
    write_cmd(self, self->fn);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mcd8544_MCD8544_power_obj, 1, 2, mcd8544_MCD8544_power);


// Doesnt fit - needs refactor
//STATIC mp_obj_t mcd8544_MCD8544_contrast(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
//    static const mp_arg_t allowed_args[] = {
//        { MP_QSTR_vop,  MP_ARG_INT, {.u_int = MCD8544_VOP_DEFAULT} },
//        { MP_QSTR_bias, MP_ARG_INT, {.u_int = MCD8544_BIAS_DEFAULT} },
//        { MP_QSTR_temp, MP_ARG_INT, {.u_int = MCD8544_TEMP_COEFF_DEFAULT} },
//    };
//    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
//    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
//
//    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
//
//    // Set the contrast and voltages
//    uint8_t vop = args[0].u_int;
//    uint8_t bias = args[1].u_int;
//    uint8_t temp = args[2].u_int;
//
//    if (vop < 0 || vop > 127) mp_raise_ValueError("Operating voltage out of range");
//    if (bias < 0 || bias > 7) mp_raise_ValueError("Bias voltage out of range");
//    if (temp < 0 || temp > 3) mp_raise_ValueError("Temperature coefficient out of range");
//
//    write_extended_cmd(self, vop, bias, temp);
//    return mp_const_none;
//}
//STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mcd8544_MCD8544_contrast_obj, 1, mcd8544_MCD8544_contrast);


STATIC mp_obj_t mcd8544_MCD8544_invert(size_t n_args, const mp_obj_t *args) {
    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1 || mp_obj_is_true(args[1])) {
        write_cmd(self, MCD8544_DISPLAY_INVERSE);
    } else {
        write_cmd(self, MCD8544_DISPLAY_NORMAL);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mcd8544_MCD8544_invert_obj, 1, 2, mcd8544_MCD8544_invert);


STATIC mp_obj_t mcd8544_MCD8544_display(size_t n_args, const mp_obj_t *args) {
    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1 || mp_obj_is_true(args[1])) {
        write_cmd(self, MCD8544_DISPLAY_NORMAL);
    } else {
        write_cmd(self, MCD8544_DISPLAY_BLANK);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mcd8544_MCD8544_display_obj, 1, 2, mcd8544_MCD8544_display);


STATIC mp_obj_t mcd8544_MCD8544_test(size_t n_args, const mp_obj_t *args) {
    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1 || mp_obj_is_true(args[1])) {
        write_cmd(self, MCD8544_DISPLAY_ALL);
    } else {
        write_cmd(self, MCD8544_DISPLAY_NORMAL);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mcd8544_MCD8544_test_obj, 1, 2, mcd8544_MCD8544_test);


STATIC mp_obj_t mcd8544_MCD8544_horizontal(size_t n_args, const mp_obj_t *args) {
    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    // vertical or horizontal addressing
    if (n_args == 1 || mp_obj_is_true(args[1])) {
        self->fn &= ~MCD8544_ADDRESSING_VERT;
    } else {
        self->fn |= MCD8544_ADDRESSING_VERT;
    }
    write_cmd(self, self->fn);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mcd8544_MCD8544_horizontal_obj, 1, 2, mcd8544_MCD8544_horizontal);


STATIC mp_obj_t mcd8544_MCD8544_position(mp_obj_t self_in, mp_obj_t x, mp_obj_t y) {
    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // set cursor to column x (0~83), bank y (0~5)
    write_cmd(self, MCD8544_COL_ADDR | (mp_obj_get_int(x) & 0x7F));  // set x pos (0~83)
    write_cmd(self, MCD8544_BANK_ADDR | (mp_obj_get_int(y) & 0x3F)); // set y pos (0~5)
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(mcd8544_MCD8544_position_obj, mcd8544_MCD8544_position);


STATIC mp_obj_t mcd8544_MCD8544_clear(mp_obj_t self_in) {
    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // clear DDRAM, reset x,y position to 0,0
    uint8_t buffer[MCD8544_BYTES]; // 4032 pixels == 504 bytes (84 cols, 48 rows)
    write_data(self, (const uint8_t*)buffer, MCD8544_BYTES);
    mcd8544_MCD8544_position(self, 0, 0);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mcd8544_MCD8544_clear_obj, mcd8544_MCD8544_clear);


STATIC mp_obj_t mcd8544_MCD8544_text(mp_obj_t self_in, mp_obj_t text) {
    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const char *str = mp_obj_str_get_str(text);

    // loop over chars
    for (; *str; ++str) {
        // get char and make sure its in range of font
        int chr = *(uint8_t*)str;
        if (chr < 32 || chr > 127) {
            chr = 127;
        }
        // get char data
        const uint8_t *chr_data = &font_petme128_8x8[(chr - 32) * 8];
        write_data(self, chr_data, 8);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mcd8544_MCD8544_text_obj, mcd8544_MCD8544_text);


STATIC mp_obj_t mcd8544_MCD8544_command(mp_obj_t self_in, mp_obj_t command) {
    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(self_in);
    write_cmd(self, (uint8_t)mp_obj_get_int(command));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(mcd8544_MCD8544_command_obj, mcd8544_MCD8544_command);


STATIC mp_obj_t mcd8544_MCD8544_data(mp_obj_t self_in, mp_obj_t data) {
    mcd8544_MCD8544_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t src;
    mp_get_buffer_raise(data, &src, MP_BUFFER_READ);
    write_data(self, (const uint8_t*)src.buf, src.len);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(mcd8544_MCD8544_data_obj, mcd8544_MCD8544_data);


STATIC const mp_rom_map_elem_t mcd8544_MCD8544_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&mcd8544_MCD8544_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&mcd8544_MCD8544_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_power), MP_ROM_PTR(&mcd8544_MCD8544_power_obj) },
    // doesnt fit - needs refactor
    //{ MP_ROM_QSTR(MP_QSTR_contrast), MP_ROM_PTR(&mcd8544_MCD8544_contrast_obj) },
    { MP_ROM_QSTR(MP_QSTR_invert), MP_ROM_PTR(&mcd8544_MCD8544_invert_obj) },
    { MP_ROM_QSTR(MP_QSTR_display), MP_ROM_PTR(&mcd8544_MCD8544_display_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_test), MP_ROM_PTR(&mcd8544_MCD8544_test_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&mcd8544_MCD8544_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_horizontal), MP_ROM_PTR(&mcd8544_MCD8544_horizontal_obj) },
    { MP_ROM_QSTR(MP_QSTR_position), MP_ROM_PTR(&mcd8544_MCD8544_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&mcd8544_MCD8544_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_command), MP_ROM_PTR(&mcd8544_MCD8544_command_obj) },
    { MP_ROM_QSTR(MP_QSTR_data), MP_ROM_PTR(&mcd8544_MCD8544_data_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mcd8544_MCD8544_locals_dict, mcd8544_MCD8544_locals_dict_table);


const mp_obj_type_t mcd8544_MCD8544_type = {
    { &mp_type_type },
    .name = MP_QSTR_MCD8544,
    .print = mcd8544_MCD8544_print,
    .make_new = mcd8544_MCD8544_make_new,
    .locals_dict = (mp_obj_dict_t*)&mcd8544_MCD8544_locals_dict,
};

mp_obj_t mcd8544_MCD8544_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_spi, ARG_dc, ARG_cs, ARG_reset, ARG_horizontal, ARG_vop, ARG_bias, ARG_temp };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_spi,        MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_dc,         MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_cs,         MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_reset,      MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_horizontal, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_vop,        MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MCD8544_VOP_DEFAULT} },
        { MP_QSTR_bias,       MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MCD8544_BIAS_DEFAULT} },
        { MP_QSTR_temp,       MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MCD8544_TEMP_COEFF_DEFAULT} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mcd8544_MCD8544_obj_t *self = m_new_obj(mcd8544_MCD8544_obj_t);
    self->base.type = &mcd8544_MCD8544_type;

    mp_obj_base_t *spi_obj = (mp_obj_base_t*)MP_OBJ_TO_PTR(args[ARG_spi].u_obj);
    self->spi_obj = spi_obj;

    if (args[ARG_dc].u_obj == MP_OBJ_NULL) {
        mp_raise_ValueError("dc pin is required");
    }
    self->dc = mp_hal_get_pin_obj(args[ARG_dc].u_obj);
    mp_hal_pin_output(self->dc);
    mp_hal_pin_write(self->dc, 0);

    if (args[ARG_cs].u_obj != MP_OBJ_NULL) {
        self->cs = mp_hal_get_pin_obj(args[ARG_cs].u_obj);
        mp_hal_pin_output(self->cs);
        mp_hal_pin_write(self->cs, 1);
    }
    if (args[ARG_reset].u_obj != MP_OBJ_NULL) {
        self->reset = mp_hal_get_pin_obj(args[ARG_reset].u_obj);
        mp_hal_pin_output(self->reset);
        mp_hal_pin_write(self->reset, 1);
    }

    // power down, horizontal addressing, basic instruction set
    self->fn = MCD8544_FUNCTION_SET | MCD8544_POWER_DOWN;

    // does not work
    //mp_map_t kw_args;
    //mcd8544_MCD8544_init(n_args, all_args, &kw_args);
    // works
    mcd8544_MCD8544_init_internal(self, args[ARG_horizontal].u_bool, args[ARG_vop].u_int, args[ARG_bias].u_int, args[ARG_temp].u_int);
    return MP_OBJ_FROM_PTR(self);
}


STATIC const mp_map_elem_t mcd8544_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_mcd8544) },
    { MP_ROM_QSTR(MP_QSTR_MCD8544), (mp_obj_t)&mcd8544_MCD8544_type },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_mcd8544_globals, mcd8544_module_globals_table );

const mp_obj_module_t mp_module_mcd8544 = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_mcd8544_globals,
};

MP_REGISTER_MODULE(MP_QSTR_mcd8544, mp_module_mcd8544, MODULE_MCD8544_ENABLED);
