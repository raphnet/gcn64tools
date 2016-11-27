# Gamecube/N64 to USB adapter management tools

## Introduction

This is the source code for a set of tools used to manage (configure, update the firmware, etc) raphnet V3 Gamecube/N64 controller to USB adapters.

Project homepage: [Tools for the 3rd generation of raphnet GC/N64 to USB adapters](http://www.raphnet.net/programmation/gcn64tools/index_en.php)

## License

The project is released under the General Public License version 3.

## Directories

 * src/ : The firmware source code
 * scripts/ : Scripts and/or useful files

## Compiling the tools

In the tool/ directory, just type make.

There are a few dependencies:
 - libhidapi-dev
 - libhidapi-hidraw0
 - pkg-config
 - gtk3 (for the gui only)

Provided you have all the dependencies installed, under Linux at least, it should
compile without errors. For other environments such has MinGW, there are provisions
in the makefile to auto-detect and tweak the build accordingly, but it if fails, be
prepared to hack the makefile.

## Using the tools

Under Linux, you should configure udev to give the proper permissions to your user,
otherwise communicating with the device won't be possible. The same requirement
also applies to dfu-programmer.

An easy way to do this is to copy the two files below to /etc/udev/rules.d, restart
udev and reconnect the devices.

scripts/99-atmel-dfu.rules
scripts/99-raphnet.rules

For information on how to actually /use/ the tools, try --help. Ex:

$ ./gcn64ctl --help
