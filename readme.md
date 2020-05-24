# MicroPython PCD8544 - WIP

⚠️ 👨‍💻 🚧 A WIP MicroPython library for the Philips PCD8544 84x48 monochrome LCD, used by the Nokia 5110 display.

## Build and deploy:

### Wemos D1 Mini (ESP8266)

```
cd micropython/ports/esp8266
make clean
make USER_C_MODULES=../../../mikes_modules CFLAGS_EXTRA=-DMODULE_MCD8544_ENABLED=1 all
make PORT=/dev/tty.wchusbserial1430 deploy
```

### ESP32 (TinyPICO)

```
cd ports/esp32
make BOARD=TINYPICO clean
make BOARD=TINYPICO

make USER_C_MODULES=../../../mikes_modules CFLAGS_EXTRA=-DMODULE_MCD8544_ENABLED=1 BOARD=TINYPICO

make BOARD=TINYPICO erase PORT=/dev/tty.SLAB_USBtoUART
make BOARD=TINYPICO deploy PORT=/dev/tty.SLAB_USBtoUART
```

⚠️ I've called it MCD8544 instead of PCD8544 temporarily, so it can co-exist with my python verison while I'm developing it.


# Testing python version

<https://github.com/mcauser/micropython-pcd8544>

```python
import pcd8544
import time
from machine import Pin, SPI

spi = SPI(1)
spi.init(baudrate=2000000, polarity=0, phase=0)
cs = Pin(2)
dc = Pin(15)
rst = Pin(0)

# backlight on
bl = Pin(12, Pin.OUT, value=0)

plcd = pcd8544.PCD8544(spi, cs, dc, rst)
plcd.data(bytearray([0xFF] * 504))
plcd.data(bytearray([0x00] * 504))

# test 1
start = time.ticks_us()
plcd.data(bytearray([0x55, 0xAA] * 42 * 6))
time.ticks_diff(time.ticks_us(), start)

# test 20x
buf = bytearray([0x55, 0xAA] * 42 * 6)
ticks_us = time.ticks_us
start = ticks_us()
for i in range(20):
    plcd.data(buf)
end = ticks_us()
duration = time.ticks_diff(end, start) / 20
print('Average over 20 tests: ', duration)
```

# Testing C version

<https://github.com/mcauser/micropython-pcd8544-c>

```python
import mcd8544
import time
from machine import Pin, SPI

spi = SPI(1)
spi.init(baudrate=2000000, polarity=0, phase=0)
cs = Pin(2, Pin.OUT)
dc = Pin(15, Pin.OUT)
rst = Pin(0, Pin.OUT)
bl = Pin(12, Pin.OUT, value=0)

# swapped dc + cs in constructor so cs can be optional
lcd = mcd8544.MCD8544(spi, dc, cs, rst)

# test 20x
buf = bytearray([0x55, 0xAA] * 42 * 6)
ticks_us = time.ticks_us
start = ticks_us()
for i in range(20):
    lcd.data(buf)
end = ticks_us()
duration = time.ticks_diff(end, start) / 20
print('Average over 20 tests: ', duration)
```

### Testing C version on TinyPICO

```python
import mcd8544
from machine import Pin, SPI

spi = SPI(2, baudrate=8000000, polarity=0, phase=0, bits=8, firstbit=0, sck=Pin(18), mosi=Pin(23), miso=Pin(19))

cs = Pin(21, Pin.OUT, value=1)
dc = Pin(32, Pin.OUT, value=1)
rst = Pin(22, Pin.OUT, value=1)
bl = Pin(33, Pin.OUT, value=1)  # backlight on

lcd = mcd8544.MCD8544(spi, dc, cs, rst)
# lcd = mcd8544.MCD8544(spi, dc, cs, rst, 0, 63, 4, 2)

# test pattern (50% on)
lcd.data(bytearray([0x55, 0xAA] * 42 * 6))

# fill
lcd.data(bytearray([0xff] * 504))

# clear
lcd.data(bytearray([0x00] * 504))

# words
lcd.position(0,0)
lcd.text('Hello')
lcd.position(0,1)
lcd.text('World')

# clear/fill
lcd.fill(0) # throws Guru Meditation Error
lcd.fill(1) # throws Guru Meditation Error

# nokia hands
lcd.position(0,0)
lcd.data(bytearray(b'\x80\x00\x00\x80\x00\x00\x80\x00\x00\x80\x00\x00\x80\x00\x00\x80\x00\x00\x80\x80\x40\x40\x40\x80\x80\xC0\xC0\x40\xC0\xA0\xE0\xC0\xE0\xE0\xF0\xF0\xF8\xF8\xF8\xFC\xFC\xFE\xEE\xF4\xF0\xF0\x70\x30\x00\x80\x00\x00\x80\x00\x0C\x9C\x1C\x38\xB8\x38\x38\xB8\xF8\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF0\xF8\xF8\xF8\xF8\x88\x20\x8A\x20\x08\x22\x08\x00\x0A\x00\x00\x02\x80\x71\xBA\xDA\xFD\xDD\xED\xDE\xEE\xF7\xFF\xFB\xFD\xFD\xFE\xFF\x7F\x3F\x1F\x9F\x3F\x7F\x6F\x0F\xAF\x1F\xBF\x3E\x3C\x7A\x78\x70\x22\x88\xA0\x2A\x80\x08\x62\xE0\xE0\xF2\xF0\x58\xDA\xF8\xFC\x92\xFE\xFF\xFF\xD3\xFF\xFD\xF3\xE1\xF0\xF9\x7F\xBF\x3F\x8F\x2F\x4F\xAF\x0F\x4F\xA7\x0F\xAF\x87\x2F\x82\x80\x20\xC0\x80\x80\x50\x40\xC4\xD0\xA0\xE8\xE4\xEA\xFF\xFB\xFD\xFF\xFF\xFF\xFF\xFF\xEF\x4F\x27\x53\xA8\x54\x29\x4A\xB5\x82\xAC\xA1\x8A\xB6\x50\x4D\x32\xA4\x4A\xB4\xA9\x4A\x52\xB4\xAA\x45\xA8\xDA\x22\xAC\xD2\x2A\x52\xA8\x52\x4C\xB0\xAD\x43\x5B\xB3\x45\xA8\x5B\xA3\xAB\x55\xA8\x52\x54\xA9\x56\xA8\x45\xBA\xA4\x49\x5A\xA2\x54\xAA\x52\xFE\xFF\xFF\xFE\xFD\xFF\xFF\xFF\xFE\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x7F\xFF\xFE\xBF\x7F\xBF\xBF\xFF\xDF\xBF\x5F\xDF\x7F\xDF\x7F\xDF\xAF\x7F\xEE\x8E\xF1\x6E\x99\xF7\x6A\xDD\xB2\x6E\xD5\x7A\xD7\xAC\x75\xDB\x6D\xD5\x7A\xD7\xAC\x7B\xE5\xDE\xA9\x77\xDA\xB5\xEE\x59\xB6\xEB\xDD\xB6\x69\xD6\xBF\xE8\x55\xEF\xB9\xD6\xED\xB5\x5B\xAB\xFF\xFD\xF7\xFF\x01\x01\x01\x01\xE1\xC1\x81\x03\x05\x0F\x1D\x2F\x7E\x01\x00\x01\x01\xFF\xFE\x03\x01\x01\x00\xF1\xF0\xF1\x71\xF1\xF1\xB1\xF1\x01\x01\x01\x03\xFE\xFF\x01\x01\x01\x01\xBE\x1B\x0D\x07\x03\x41\xE1\xF1\xF9\x6D\xFF\xFF\x00\x01\x01\x01\xFF\xFF\xEB\x3E\x0D\x03\x01\x41\x71\x70\x41\x01\x03\x0E\x3B\xEF\xFE\xFB\xEE\x7D\xF7\xFF\xFF\xFF\xFF\xFE\xFF\xF0\xF0\xF0\xF0\xFF\xFF\xFF\xFF\xFE\xFC\xF8\xF0\xF0\xF0\xF0\xF0\xF0\xFF\xFF\xF8\xF0\xF0\xF0\xF1\xF1\xF1\xF1\xF1\xF1\xF1\xF1\xF0\xF0\xF0\xF8\xFF\xFF\xF0\xF0\xF0\xF0\xFF\xFF\xFE\xFC\xF8\xF0\xF0\xF1\xF3\xF7\xFF\xFF\xF0\xF0\xF0\xF0\xFF\xF3\xF0\xF0\xF0\xFC\xFC\xFC\xFC\xFC\xFC\xFC\xFC\xF0\xF0\xF0\xF3\xFF\xFF\xFF\xFF\xFF'))
```

## Tested, works:

```
# lcd.power(on)
lcd.power(False)
lcd.power(0)
lcd.power(True)
lcd.power(1)

# lcd.display(on)
lcd.display(False)
lcd.display(0)
lcd.display(True)
lcd.display(1)

# lcd.invert(inverted)
lcd.invert(False)
lcd.invert(0)
lcd.invert(True)
lcd.invert(1)

# lcd.test(testing)
lcd.test(False)
lcd.test(0)
lcd.test(True)
lcd.test(1)

# lcd.position(x, y)
lcd.position(0,0)
lcd.data(bytearray([0xff]))
lcd.position(1,1)
lcd.position(0,3)

# lcd.init(addressing, vop, bias, temp)
lcd.init(0)
lcd.init(1)
lcd.init(1,63)
lcd.init(1,53,4)
lcd.init(1,63,4,2)

lcd.position(0,0)
lcd.data(bytearray([0xff]))
lcd.data(bytearray([0x55, 0xAA] * 42 * 6))  # 50% on
lcd.data(bytearray([0xFF] * 84))   # page on
lcd.data(bytearray([0xFF] * 504))  # all on
lcd.data(bytearray([0x00] * 504))  # all off

lcd.position(0,0)
lcd.text('My PCD8544')
lcd.position(0,1)
lcd.text('driver')

lcd.reset()
lcd.init()

lcd.init()
lcd.display(0)
lcd.position(0,0)
lcd.text('Hello')
lcd.display(1)

lcd.init()
lcd.power(0)
lcd.position(0,0)
lcd.text('World')
lcd.power(1)

lcd.fill(0) # throws Guru Meditation Error
lcd.fill(1) # throws Guru Meditation Error
```

## WIP

```python
lcd.fill(colour)
  # crashes and resets
  # this writes zeros to the ddram - its faster to just reset() it - same outcome, but need to persist horiz, vop, bias and temp

  # Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled.
  # Core 1 register dump:
  # PC      : 0x400e17a5  PS      : 0x00060130  A0      : 0x800fc211  A1      : 0x3ffce7f0
  # A2      : 0x00000000  A3      : 0x00000001  A4      : 0x3ffce830  A5      : 0x00000000
  # A6      : 0x00000008  A7      : 0x3ffce9b0  A8      : 0x00000000  A9      : 0x00000015
  # A10     : 0x00000000  A11     : 0x00000000  A12     : 0x00000028  A13     : 0x3ffce7e8
  # A14     : 0x00000002  A15     : 0x00000002  SAR     : 0x00000001  EXCCAUSE: 0x0000001c
  # EXCVADDR: 0x00000000  LBEG    : 0x40093cb0  LEND    : 0x40093cde  LCOUNT  : 0xffffffff
  #
  # ELF file SHA256: 0000000000000000000000000000000000000000000000000000000000000000
  #
  # Backtrace: 0x400e17a5:0x3ffce7f0 0x400fc20e:0x3ffce810 0x400fc275:0x3ffce830 0x400e3fc9:0x3ffce860 0x400dfda5:0x3ffce880 0x400dfed9:0x3ffce8a0 0x400ede63:0x3ffce8c0 0x400e4124:0x3ffce960 0x400dfda5:0x3ffce9d0 0x400dfdce:0x3ffce9f0 0x4010751b:0x3ffcea10 0x40107735:0x3ffceaa0 0x400f71ac:0x3ffceae0 0x4009453a:0x3ffceb10
  #
  # Rebooting...


lcd.contrast(vop, bias, temp)
  # decided not to include this, instead call init(addressing, vop, bias, temp)

lcd.text()
  # simple version working, using stm32 petme font
  # be nice if i could add an inverted version
  # add x and y args and internally set position before printing
```

## Connections

WeMos D1 Mini | PCD8544 LCD
------------- | ----------
D3 (GPIO0)    | 1 RST
D4 (GPIO2)    | 2 CE
D8 (GPIO15)   | 3 DC
D7 (GPIO13)   | 4 Din
D5 (GPIO14)   | 5 Clk
3V3           | 6 Vcc
D6 (GPIO12)   | 7 BL
G             | 8 GND

TinyPICO | PCD8544 LCD
-------- | ----------
GPIO22   | 1 RST
GPIO21   | 2 CE
GPIO32   | 3 DC
GPIO23   | 4 Din
GPIO18   | 5 Clk
3V3      | 6 Vcc
GPIO33   | 7 BL
G        | 8 GND

## License

Licensed under the [MIT License](http://opensource.org/licenses/MIT).
