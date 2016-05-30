# KittyFeeder2
Version 2 of the Kitty Feeder
A much improved version of the [Arduino-Cat-Feeder](https://github.com/wizard97/Arduino-Cat-Feeder)

Some features include:
- Peltier refrigeration
- LCD Menu System
- Automatic temperature control
- WiFi web interface

Due to memory flash and memory limitations, it will only run on the AtMega2560. Also, there are several libraries that need to be downloaded, look at the included header files and download the approriate libraries into your libraries folder. One exception to this is the arduino-menu-system library, I forked the official repo and created my own version to support display callback methods, you can find it here https://github.com/wizard97/arduino-menusystem.
