# Improvement list

## Local improvement list
* Debug the for Arduino Nano Every. The prpogram hangs at some point and I suspect this is because incompatibilities with some library.
* A menu option to enable auto-strech of the bar graph respect to the maximum value read.
* A button action to take a snaphsot of the values.
* A button action for HOLD mode
		- Pressing Joy BUTTON in the display spectra/data screen enters the HOLD mode
		- Pressing again Joy BUTTON exist HOLD mode

## Bluetooth improvement list
* Implement a command systems from the mobile applicatiion (Which commands are necessary?)
 - Emulate the local menu? 
 - Add brightness control? 
 - Debug to serial ('x' and 'z') option?

## Notes
- In Hold mode, repeat the last measurement every second more or less. This means same milis() value but different sequence numbers
  Do the same for Luxometer.

