# Spectral Colors (Release 1)

Low Cost, Low Resolution 6 band spectrograph Arduno project based on the AS7262 device. 
This educational project is devveloped under EU-funded ACTION project.

## Notes

Release 1 is a prototype made with:
* an standard Arduino Nano 328P.
* An AS7272 spectral sensor
* A miniTFTwing display

The built-in LED is attached to pin Arduino Nano D13. 
However, this pin is used to interface miniTCFWing via SPI
So, the built-in LED becomes unusable after miniTFTWing initialization

Interrupts from the spectral via the INT pin do not work (they never happen)
So we have removed this connection and the handling software routines.

Connections are the following:

```
              Arduino Nano        AS7262 Spectral Sensor
              ============        ======================

                    /
                   |  A4 (SDA)  -----> SDA 
              I2C  |  A5 (SCL)  -----> SCL 
                   \

                   /
                   |   3.3V    <====> 3.3V 
              Pwr  |   GND     <====> GND
                   \

              Arduino Nano        miniTFTWing
              ============        ===========
                   /
                   | D11 (MOSI) -----> MOSI
              SPI <  D13 (SCK)  -----> SCLK
                   |     D5     -----> CS
                   |     D6     -----> DC
                   \

                   /
                   |  A4 (SDA)  -----> SDA (+10k pullup)
              I2C  |  A5 (SCL)  -----> SCL (+10k pullup)
                   \

                   /
                   |   3.3V    <====> 3.3V 
              Pwr  |   GND     <====> GND
                   \

```

