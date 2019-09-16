# Spectral Colors

Low Cost, Low Resolution 6 band spectrograph Arduno project based on the AS7262 device.
This educational project is devveloped under EU-funded ACTION project.

## Notes

The current version is based on:
* [Arduino Nano](https://store.arduino.cc/arduino-nano) or [Arduino Nano Every](https://store.arduino.cc/nano-every)
* [Adafruit AS7262 6-Channel Visible Light / Color Sensor Breakout](https://www.adafruit.com/product/3779)
* [Adafruit Mini Color TFT with Joystick FeatherWing](https://www.adafruit.com/product/3321)
* [Adafruit Bluefruit LE SPI Friend - Bluetooth Low Energy (BLE)](https://www.adafruit.com/product/2633)

## Power Supply Issues

* Apparently, the *Adafruit Mini Color TFT with Joystick FeatherWing* works only on 3.3V, but its inputs are 5V-tolerant although this is not documented.

3.3V power supply is currently taken from the Arduino Nano board itself. In fact, it is the USB-to-Serial converter which generates this voltage, and apparently only 50mA are only available.

* All other breakout modules have internal level shifters for the data lines and/or regulated down-converters from %V to 3.3V. In particular:
- a pad in back of the Bluetooth LE breakout can deliver 250mA @ 3.3V.
- the 3V3 pin of the AS7262 board can deliver 100mA @ 3.3V.

## I2C Pull-up resistor

* The schematic of *Adafruit AS7262 6-Channel Visible Light / Color Sensor Breakout* shows a bidirectional level shifter with MOSFETs and 10k pull-up resistors in the I2C SCL and SDA lines. So the project does not include any external pull-up resistor.