# Spectral Colors

Low Cost, Low Resolution 6 band spectrograph Arduno project based on the AS7262 device.
This educational project is devveloped under EU-funded ACTION project.

## Notes

The current version is based on the following hardware:
* [Arduino Nano](https://store.arduino.cc/arduino-nano) or [Arduino Nano Every](https://store.arduino.cc/nano-every)
* [Adafruit AS7262 6-Channel Visible Light / Color Sensor Breakout](https://www.adafruit.com/product/3779)
* [OPT3001 Digital Ambient Light Sensor Breakout](https://www.tindie.com/products/closedcube/opt3001-digital-ambient-light-sensor-breakout/)
* [Adafruit Mini Color TFT with Joystick FeatherWing](https://www.adafruit.com/product/3321)
* [Adafruit Bluefruit LE SPI Friend - Bluetooth Low Energy (BLE)](https://www.adafruit.com/product/2633)

## Features

The main features of this release are:
* Reads AS7262 & OPT3001 sensors
* Display sensor readings on a color mini TFT display (bar graph in case of AS7262).
* A simple menu system allows you to configurate AS7262 gain & exposure time
* Sends readings by Bluetooth and optionally by serial port.

## Limitations

As the Arduino Nano flash memory is almost full (98%), we cannot include features like:

* commands via BT or serial port to configure the device in the same way as done from the TFT desplay.
* accumulation of AS7262 sensor readings to boost sensitivity in low light conditions.
* a better menu system
* etc.

A future release based sole on the Arduino Nano Every will include the above features.

## Power Supply Issues

* Apparently, the *Adafruit Mini Color TFT with Joystick FeatherWing* works only on 3.3V, but its inputs are 5V-tolerant although this is not documented.

3.3V power supply is currently taken from the Arduino Nano board itself. In fact, it is the USB-to-Serial converter which generates this voltage, and apparently only 50mA are only available.

* All other breakout modules have internal level shifters for the data lines and/or regulated down-converters from %V to 3.3V. In particular:
- a pad in back of the Bluetooth LE breakout can deliver 250mA @ 3.3V.
- the 3V3 pin of the AS7262 board can deliver 100mA @ 3.3V.

## I2C Pull-up resistor

* The schematic of *Adafruit AS7262 6-Channel Visible Light / Color Sensor Breakout* shows a bidirectional level shifter with MOSFETs and 10k pull-up resistors in the I2C SCL and SDA lines. So the project does not include any external pull-up resistor.

## Sensor data sent to Serial Port

It is possible to enable/disable the transmission of both sensors' readings by serial port at 115200 baud.
* Sending 'x' -> Enable
* Sending 'z' -> Disable

This may be useful for direct PC data gathering (i.e device calibration)

## Data Format

Readings are sent in a compact but still valid JSON format as a sequence of values so that you can easily decode them using your favourite programming language (i.e. Python). The frequency of each sensor message is inversely proportional to the device exposure time. 

### Data format for OPT3001 sensor readings

| Index |  Type  | Units | Description                       |
|:-----:|:------:|:-----:|:--------:|:----------------------------------|
| 0     | string |   -   | Sensor Type. "O" = OPT3001. |
| 1     | int    |   -   | Message sequence number. Useful to detect missing messages. |
| 2     | int    |   ms  | Relative timestamp since the Arduino boot. |
| 3     | int    |   ms  | Sensor exposure time. | 
| 4     | float  |   lux | Light intensity as perceived by the human eye. |

Example:
```
["O",18,16920,800,96.48]
```
The decoding of such message is shown:

* "O" => OPT3001 sensor
* 18 => Sequence number
* 16920 => milliseconds since power on
* 800 => OPT3001 exposure time (ms)
* 96.48 => Light intensity (lux)


### Data format for AS7262 sensor readings


| Index |  Type  | Units | Description                       |
|:-----:|:------:|:-----:|:--------:|:----------------------------------|
| 0     | string |   -   | Sensor Type. "A" = AS7262, "O" = OPT3001. |
| 1     | int    |   -   | Message sequence number. Useful to detect missing messages. |
| 2     | int    |   ms  | Relative timestamp since the Arduino boot. |
| 3     | float  |   ms  | Sensor exposure time. | 
| 4     | float  |   -   | Sensor gain. |
| 5     | int    |  ºC   | AS7262 internal temperature. |
| 6     | float  | counts/μW/cm2 | Violet (450nm) calibrated value. |
| 7     | int    | counts/μW/cm2 | Violet (450nm) raw value. |
| 8     | float  | counts/μW/cm2 | Blue (500nm) calibrated value. |
| 9     | int    | counts/μW/cm2 | Blue (500nm) raw value. |
| 10    | float  | counts/μW/cm2 | Green (550nm) calibrated value. |
| 11    | int    | counts/μW/cm2 | Green (550nm) raw value. |
| 12    | float  | counts/μW/cm2 | Yellow (570nm) calibrated value. |
| 13    | int    | counts/μW/cm2 | Yellow (570nm) raw value. |
| 14    | float  | counts/μW/cm2 | Orange (600nm) calibrated value. |
| 15    | int    | counts/μW/cm2 | Orange (600nm) raw value. |
| 16    | float  | counts/μW/cm2 | Red (650nm) calibrated value. |
| 17    | int    | counts/μW/cm2 | Red (650nm) raw value. |


Example:
```
["A",45,16869,280.0,64,27,49.10,42,65.76,54,167.21,150,100.00,105,127.00,126,56.35,67]
```
The decoding of such message is shown:

* "A" => AS7262 sensor
* 45 => Sequence number
* 16869 => milliseconds since power on
* 280.0 => AS7262 exposure time (ms)
* 64 => AS7262 gain
* 27 => AS7262 temperature (ºC)
* 49.10 => Violet (calibrated)
* 42 => Violet (raw) 65.76 => Blue (calibrated)
* 54 => Blue (raw)
* 167.21 => Green (calibrated)
* 150 => Green (raw)
* 100.00 => Yellow (calibrated)
* 105 => Yellow (raw)
* 127.00 => Orange (calibrated)
* 126 => Orange (raw)
* 56.35 => Red (calibrated)
* 67 => Red (raw)

