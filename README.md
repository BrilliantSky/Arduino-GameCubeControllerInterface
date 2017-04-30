# Arduino-GameCubeControllerInterface
A library for interfacing a Nintendo GameCube Controller to the Arduino.

## Intended Usage
This project is intended to allow an Arduino project to be controlled using
a single GameCube controller. It was created to allow control of an RC robot
with a controller that I had on hand.

# Acknowledgements

This code is derived from work by Andrew Brown:
https://github.com/brownan/Gamecube-N64-Controller

Specifically, this project removes the N64 parts and provides a clean C++ class
for easy integration with other code. For more details, or if you encounter issues,
see the above link for a more thorough explanation of how the code works.

I also found this link helpful: http://www.int03.co.uk/crema/hardware/gamecube/gc-control.html.

# Electrical Connections
I used an LM317 adjustable regulator and 12V wall wart to create a 3.3 volt rail for the controller.
Connect controller data line to arduino pin D2. This is hardcoded and cannot be changed.
Connect 1k resistor between data line and controller +3.3v supply.
+5V to the controller is not required unless you want to use the rumble feature (at least on my controller).

# Usage and architecture

See examples/gamecube_serial/gamecube_serial.ino for an example program that
prints out the controller data every second.

The controller must be polled to check the status of buttons, joysticks, etc.
This is done via an update() method, which can be called periodically or
as needed. After a call to update(), button and joystick data is available
via various functions. A zero-adjust feature for the joysticks is provided.
Rumble is supported.

A note about timing: The gamecube controller is bitbanged and requires precise
timing to work. Therefore, interrupts will be temporarily disabled during
calls to initialize() and update(). The timing may need adjustment for processors
other than the AtMega328P, or if your clock speed is not 16MHz.

