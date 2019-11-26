# Improvement list

## Local improvement list
* Debug the for Arduino Nano Every. The prpogram hangs at some point and I suspect this is because incompatibilities with some library.
* Backlight screen out of the menu system
* A menu option to enable auto-strech of the bar graph respect to the maximum value read.
* A button action to take a snaphsot of the values.
* A button action to cycle display between bars and values
		- Pressing Joy UP/DOWN swicthes between bars mode and text mode
		- Pressing Joy BUTTON in the display spectra screen enters the HOLD mode
		- Pressing again Joy BUTTON exist HOLD mode

## Bluetooth improvement list
* Implement a command systems from the mobile applicatiion (Which commands are necessary?)
 - Emulate the local menu? 
 - Add brightness control? 
 - Debug to serial ('x' and 'z') option?

## Notes
- milis() timestamp at the end of the measurement, not in the transmission
- sequence number in the transmission. Its purpose is to detect transmission errors.
- In Hold mode, repeat the last measurement every second more or less. This means same milis() value but different sequence numbers
  Do the same for Luxometer.

