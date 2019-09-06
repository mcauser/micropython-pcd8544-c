# MicroPython PCD8544 - WIP

A WIP MicroPython library for the Philips PCD8544 84x48 monochrome LCD, used by the Nokia 5110 display.

## Build and deploy:

```
cd micropython/ports/esp8266
make clean
make USER_C_MODULES=../../../mikes_modules CFLAGS_EXTRA=-DMODULE_MCD8544_ENABLED=1 all
make PORT=/dev/tty.wchusbserial1430 deploy
```

I've called it MCD8544 instead of PCD8544 temporarily, so it can co-exist with my python verison while I'm developing it.


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

## Tested, works:

```
lcd.power(False)
lcd.power(0)
lcd.power(True)
lcd.power(1)
lcd.power()
lcd.display(False)
lcd.display(0)
lcd.display(True)
lcd.display(1)
lcd.display()
lcd.invert(False)
lcd.invert(0)
lcd.invert(True)
lcd.invert(1)
lcd.invert()
lcd.test(False)
lcd.test(0)
lcd.test(True)
lcd.test(1)
lcd.test()
lcd.horizontal(False)
lcd.horizontal(0)
lcd.horizontal(True)
lcd.horizontal(1)
lcd.horizontal()
lcd.position(0,0)
lcd.position(1,1)
lcd.position(0,3)
lcd.command(0x20)
lcd.data(bytearray([0xff]))
lcd.data(bytearray([0x55, 0xAA] * 42 * 6))  # 50% on
lcd.data(bytearray([0xFF] * 48))   # block
lcd.data(bytearray([0xFF] * 504))  # all on
lcd.data(bytearray([0x00] * 504))  # all off
```

## WIP

```python
lcd.reset()
  # does nothing... ?

lcd.clear()
  # crashes and resets
  # this writes zeros to the ddram - its faster to just reset() it - same out come, but need to persist horiz, vop, bias and temp

lcd.contrast()
  # did not fit, commented out for now

  # manually set it to 63
  lcd.command(0x20 | 0x01)
  lcd.command(0x04 | 2) # temp
  lcd.command(0x10 | 4) # bias
  lcd.command(0x80 | 63) # vop
  lcd.command(0x20)
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
G             | 8 Gnd

## License

Licensed under the [MIT License](http://opensource.org/licenses/MIT).
