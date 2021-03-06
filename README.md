# StreetColors

Low Cost, Low Resolution 6 band photometer Arduno project based on the AS7262 device.
This educational project is developped under EU-funded [ACTION project](https://actionproject.eu/).

## Features

The main features of this release are:
* Reads AS7262 & OPT3001 sensors.
* Continuous real-time readings or HOLD mode.
* Accumulate readings for low level light situations.
* Configuration screens:
	- Exposure time (5.6 ms to 1428ms, 25ms steps)
	- Gain (1x, 3.7x, 16x, 64x)
	- Accumulation factor (1x, 2x, 4x, 8x, 16x)
* Display screens:
	- Luxometer screen (OPT 3001)
	- 6 Color bar graph (AS7262)
	- Tabular data, 2 columns, 3 values (AS7262)
* Sends readings by Bluetooth and optionally by serial port.

## Hardware

The current version is based on the following hardware:
* [Arduino Nano](https://store.arduino.cc/arduino-nano)
* [Adafruit AS7262 6-Channel Visible Light / Color Sensor Breakout](https://www.adafruit.com/product/3779)
* [OPT3001 Digital Ambient Light Sensor Breakout](https://www.tindie.com/products/closedcube/opt3001-digital-ambient-light-sensor-breakout/)
* [Adafruit Mini Color TFT with Joystick FeatherWing](https://www.adafruit.com/product/3321)
* [Adafruit Bluefruit LE SPI Friend - Bluetooth Low Energy (BLE)](https://www.adafruit.com/product/2633)

## Libraries

This project has been built using the following libraries and versions. 
Use the Arduino Board Manager to install them:


| Library                            | Version | Comment
|:----------------------------------:|:-------:|:--------------------------|
| Adafruit BluefruitLE nRF51         |  1.9.6  |                           |
| Adafruit seesaw Library            |  1.2.0  |                           |
| Adafruit ST7735 and ST7789 Library |  1.5.6  |                           |
| Adafruit GFX Library               |  1.7.1  |                           |
| ClosedCube OPT3001                 |  1.1.2  |                           |
| Adafruit AS726X                    |  1.0.2  | *See Note 1*              |


***Note 1:*** For the Arduino Nano, I have produced a slightly more compact from for this library, so that the entrire program can fit in the Nano Flash memory. For the Nano Every, you can use the standard 
library.

## Limitations

As the Arduino Nano flash memory is almost full (99%), we cannot include features like:

* commands via BT or serial port to configure the device in the same way as done from the TFT desplay.
* a better menu system
* etc.

A future release based on the pin-to-pin compatible [Arduino Nano Every](https://store.arduino.cc/nano-every) will include the above features.

## Power Supply Issues

* Apparently, the *Adafruit Mini Color TFT with Joystick FeatherWing* works only on 3.3V, but its inputs are 5V-tolerant although this is not documented.

3.3V power supply is *not taken from the Arduino Nano* board itself. In fact, it is the USB-to-Serial converter which generates this voltage, and apparently only 50mA are only available.

* All other breakout modules have internal level shifters for the data lines and/or regulated down-converters from %V to 3.3V. In particular:
- a pad in back of the Bluetooth LE breakout can deliver 250mA @ 3.3V.
- the 3V3 pin of the AS7262 board can deliver 100mA @ 3.3V. ***This is where we take the 3.3V from.***

## I2C Pull-up resistor

* The schematic of *Adafruit AS7262 6-Channel Visible Light / Color Sensor Breakout* shows a bidirectional level shifter with MOSFETs and 10k pull-up resistors in the I2C SCL and SDA lines. 
* Arduino Wire Library is configured to use internal (weak) pull ups.

So the project does not include any external pull-up resistor.

## Sensor data sent to Serial Port

It is possible to enable/disable the transmission of both sensors' readings by serial port at 115200 baud.
* Sending 'x' -> Enable
* Sending 'z' -> Disable

This may be useful for direct PC data gathering (i.e device calibration)

## Data Format

Readings are sent in a compact but still valid JSON format as a sequence of values so that you can easily decode them using your favourite programming language (i.e. Python). The frequency of each sensor message is inversely proportional to the device exposure time. 

### Data format for OPT3001 sensor readings

| Index |  Type  | Units | Description                                                 |
|:-----:|:------:|:-----:|:------------------------------------------------------------|
| 0     | string |   -   | Sensor Type. "O" = OPT3001.                                 |
| 1     | int    |   -   | Message sequence number. Useful to detect missing messages. |
| 2     | int    |   ms  | Relative timestamp since the Arduino boot.                  |
| 3     | int    |   -   | Number of readings accumulated.                             |
| 4     | int    |   ms  | Sensor exposure time (individual reading).                  | 
| 5     | float  |   lux | Light intensity as perceived by the human eye.              |

Example:
```
["O",18,16920,1,800,96.48]
```
The decoding of such message is shown:

* "O" => OPT3001 sensor
* 18 => Sequence number
* 16920 => milliseconds since power on
* 1 => Accumulated readings (1=only one)
* 800 => OPT3001 exposure time (ms)
* 96.48 => Light intensity (lux)


### Data format for AS7262 sensor readings


| Index |  Type  | Units | Description                                  |
|:-----:|:------:|:-----:|:---------------------------------------------|
| 0     | string |   -   | Sensor Type. "A" = AS7262, "O" = OPT3001. |
| 1     | int    |   -   | Message sequence number. Useful to detect missing messages. |
| 2     | int    |   ms  | Relative timestamp since the Arduino boot. |
| 3     | int    |   -   | Number of readings accumulated. |
| 4     | float  |   ms  | Sensor exposure time (individual reading). | 
| 5     | float  |   -   | Sensor gain. |
| 6     | int    |  ºC   | AS7262 internal temperature. |
| 7     | int    | counts/μW/cm2 | Violet (450nm) raw value. |
| 8     | int    | counts/μW/cm2 | Blue (500nm) raw value. |
| 9     | int    | counts/μW/cm2 | Green (550nm) raw value. |
| 10    | int    | counts/μW/cm2 | Yellow (570nm) raw value. |
| 11    | int    | counts/μW/cm2 | Orange (600nm) raw value. |
| 12    | int    | counts/μW/cm2 | Red (650nm) raw value. |


Example:
```
["A",45,16869,1,280.0,64,27,42,54,150,105,126,67]
```
The decoding of such message is shown:

* "A"   => AS7262 sensor
* 45    => Sequence number
* 16869 => milliseconds since power on
* 1     => Accumulated readings (1=only one reading)
* 280.0 => AS7262 exposure time (ms)
* 64    => AS7262 gain
* 27    => AS7262 temperature (ºC)
* 42    => Violet (raw) 
* 54    => Blue (raw)
* 150   => Green (raw)
* 105   => Yellow (raw)
* 126   => Orange (raw)
* 67    => Red (raw)
