## What is this?

This is the firmware for the LED bar located in _TBD_.

_TBD include a picture of the device in question_

## What does the hardware look like?

The LED bar is controlled by a Pi Pico W. There is a short USB cable on the back that you can
use to connect to the Pico. The Pico can be reset and programmed while the bar is attached to
power.

## How do I build this firmware?

First, you'll need to set up a working Pi Pico development environment.
https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf

You can use cmake to build the firmware as you would any other Pico project.

## How do I reprogram the Pico?

When you plug in the Pico, it appears as a standard serial device. You can reset it by setting
the baud rate on that device to 1200. In Linux, this looks like:

> stty -F /dev/ttyACM0 1200

This will put the Pico in bootloader mode. In bootloader mode, it looks a mass storage device.
Most OSes and environments will mount this automatically. If you're using a Linux environment
that does not do so, you can mount it through udisks2 with a command like:

> udisksctl mount -b /dev/disk/by-label/RPI-RP2 

You can then copy the .uf2 file with the new firmware into the new device. The Pico should
immediately reboot into the new firmware.

## TODO

* Better font please
* Handle on/off MQTT messages
* Handle text display MQTT messages
* .beats time
