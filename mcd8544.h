#ifndef __MCD8544_H__
#define __MCD8544_H__

// version 0.0.3

#ifdef __cplusplus
extern "C" {
#endif

// Function set 0010 0xxx
#define MCD8544_FUNCTION_SET     32 // 0x20
#define MCD8544_POWER_DOWN       4  // 0x04
#define MCD8544_ADDRESSING_VERT  2  // 0x02
#define MCD8544_EXTENDED_INSTR   1  // 0x01

// Display control 0000 1x0x
#define MCD8544_DISPLAY_BLANK    8  // 0x08
#define MCD8544_DISPLAY_ALL      9  // 0x09
#define MCD8544_DISPLAY_NORMAL   12 // 0x0c
#define MCD8544_DISPLAY_INVERSE  13 // 0x0d

// Temperature coefficient 0000 01xx
#define MCD8544_TEMP_COEFF          4 // 0x04
#define MCD8544_TEMP_COEFF_DEFAULT  2 // TC2, 17mV/K

// LCD bias voltage 0001 0xxx
#define MCD8544_BIAS          16 // 0x10
#define MCD8544_BIAS_DEFAULT  4  // Bias voltage 1:40

// Set operation voltage (Vop) 1xxx xxxx
#define MCD8544_VOP          128 // 0x80
#define MCD8544_VOP_DEFAULT  63  // Contrast set to around 50%

// DDRAM addresses
#define MCD8544_COL_ADDR   128 // 0x80 x pos (0~83)
#define MCD8544_BANK_ADDR  64  // 0x40 y pos, in banks of 8 rows (0~5)

// Display dimensions
#define MCD8544_WIDTH   84  // 0x54
#define MCD8544_HEIGHT  48  // 0x30 (6 banks of 8 vertically stacked pixels)

#ifdef  __cplusplus
}
#endif /*  __cplusplus */

#endif  /*  __MCD8544_H__ */
