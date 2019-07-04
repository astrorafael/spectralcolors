# Spectral Colores

Low Cost, Low Resolution 6 band spectrograph Arduno project based on the AS7262 device. 
This educational project is devveloped under EU-funded ACTION project.

## Notes

The current version is based on:
* Adafruit Feather 32u4 Bluefruit LE
* AS7262 spectral sensor
* Adafruit miniTFTwing display

No internal pullups exists in the 32u4, 
so external resistors are needed for the I2C bus.


The internal BL module and the TFT display share the SPI bus.

