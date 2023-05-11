 # find_devices

Audio device and serial ports search utility.

Use this utility to search for audio devices and serial ports.

The functionally of this utility is focused for ham radio use, to help with integration and automation with Direwolf.

`./find_devices -p --print "audio,ports" --audio-begin --desc Texas --audio-end --port-begin --desc cp21 --port-end`

![image](https://github.com/iontodirel/find_devices/assets/30967482/16e63b8e-40b7-43b4-9cbe-212f95ac3c2d)


## Motivation

- Easy and quick enumeration of audio devices and serial ports within one tool
- Programmable support, with easy integration and no scripting and text processing required
  - Do not need to parse and manipulate text from logs or output from aplay and arecord
  - Output from the program can directly be consumed by other programs
  - JSON output writing and printing for easy programmability, with tools like `jq`
  - Project is well structured (subjectively) and hackable for future modifications
- Correlation between audio devices and serial ports
  - A big problem for hams is finding a serial port on the same USB hub as the USB sound card
  - Easily find USB sound cards on the same hub as a USB serial port.  
- Repeatable results, across system restarts, or when using generic sound cards with the same USB descriptors.
  - A big problem for hams is using multiple sound cards that have the same USB descriptors
  - This tool aims at reliably finding devices and addresing indovidually even if the USB descriptors are all the same
- No system modifications requirements, like adding udev rules

## Table of contents

- [Motivation](#motivation)
- [Limitations](#limitations)
- [Basic example usage](#basic-example-usage)
  - [Obtaining audio devices with Alsa](#obtaining-audio-devices-with-alsa)
  - [Obtaining audio devices with find_devices](#obtaining-audio-devices-with-find_devices)
- [JSON parsing example with jq](#json-parsing-example-with-jq)
- [Building](#building)
  - [Dependencies](#dependencies)
  - [Dependencies](#development)
  - [Github Actions](#github-actions)
- [Practical Examples](#practical-examples)
- [Strategies for finding audio devices and serial ports](#strategies-for-finding-audio-devices-and-serial-ports)

## Limitations

- Only Linux is currently supported.
- Only USB serial ports are currently supported.
- Limited support for built in sound cards, like PCI based sound cards.
- Requires a Linux kernel with udev support.

## Basic example usage

### Obtaining audio devices with Alsa

To retrieve Alsa capture and playback devices:

`arecord -l`\
**** List of CAPTURE Hardware Devices ****\
card 1: CODEC [USB AUDIO  CODEC], device 0: USB Audio [USB Audio]\
&nbsp;&nbsp;Subdevices: 0/1\
&nbsp;&nbsp;Subdevice #0: subdevice #0

`aplay -l`\
**** List of PLAYBACK Hardware Devices ****\
card 1: CODEC [USB AUDIO  CODEC], device 0: USB Audio [USB Audio]\
&nbsp;&nbsp;Subdevices: 0/1\
&nbsp;&nbsp;Subdevice #0: subdevice #0

### Obtaining audio devices with find_devices

#### To retrieve audio capture and playback devices in JSON format

`./find_devices --type 'capture&playback' --json`\
{\
&nbsp;&nbsp;&nbsp;&nbsp;"audio_devices": [ \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;{ \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"card_id": 1, \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"device_id": 0, \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"plughw_id": "plughw:1,0",  \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"hw_id": "hw:1,0",  \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"name": "USB Audio Device",  \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"description": "C-Media Electronics Inc. USB Audio Device at usb-0000:00:14.0-6", \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"type": "capture&playback" \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"bus_number": "1"\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"device_number": "10",\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"id_product": "0014",\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"id_vendor": "0d8c",\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"manufacturer": "C-Media Electronics Inc.",\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"path": "/sys/devices/pci0000:00/0000:00:14.0/usb1/1-6/1-6:1.0/sound/card1",\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"hw_path": "/sys/devices/pci0000:00/0000:00:14.0/usb1/1-6",\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"product": "USB Audio Device",\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"topology_depth": "2"\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;} \
&nbsp;&nbsp;&nbsp;&nbsp;] \
}

#### To retrieve audio capture and serial ports and print them to stdout

Simply run with no arguments `./find_devices`

![image](https://github.com/iontodirel/find_devices/assets/30967482/36f088c3-a332-4329-aaff-eeb28c45b7ee)

#### To print detailed information about each device, add the `-p` argument

![image](https://github.com/iontodirel/find_devices/assets/30967482/7c017b89-581f-465d-89ef-8966e4a327f9)

## JSON parsing example with jq

Install `jq`.

Run `find_devices`:

`./find_devices --name "USB Audio" --desc "Texas Instruments" --type 'capture&playback' --json | jq -r ".devices[0].plughw_id" ` \
plughw:1,0

## Building

Install the dependencies listed in `install_dependencies.sh`.

Clone this repo with `git clone https://github.com/iontodirel/find_devices.git`.

Generate your build scripts with CMake, and build:

~~~~
cd find_devices/
mkdir out
cd out
cmake ..
make
~~~~

### Dependencies

This project uses `libudev` and `libsound2` as development library dependencies, which need to be installed with your system package manager.

This project is using `CMake`, and has been tested with the GCC compiler.

This project also utilizes the `fmt` and `nlohmann::json` libraries, which are automatically installed with the CMake build scripting.

### Development

You can use `Visual Studio` or `VSCode` for remote Linux development.

My setup include a Linux machine running Ubuntu 22.03 Desktop. I do the development in Windows in VSCode or Visual Studio.

In VSCode have CMake Tools, Cpp Tools, and Remote - SSH tools installed. They provide the developer experience for editing, building and debugging.

### Github Actions

A build action automatically builds the project code commits. This makes sure the project builds successfully and the build is well maintained.

## Help

`./find_devices --help`\
find_devices - Alsa Device finding utility

Usage:\
&nbsp;&nbsp;&nbsp;&nbsp;find_devices [--name <Name>][--desc <Description>][--type <TypeSpecifier>][--list][--verbose][--no-verbose][--help]

Options:\
&nbsp;&nbsp;&nbsp;&nbsp;--name <name>            partial or complete name of the audio device\
&nbsp;&nbsp;&nbsp;&nbsp;--desc <description>     partial or complete description of the audio device\
&nbsp;&nbsp;&nbsp;&nbsp;--verbose                enable verbose printing from this utility\
&nbsp;&nbsp;&nbsp;&nbsp;--no - verbose           machine parsable output\
&nbsp;&nbsp;&nbsp;&nbsp;--help                   print this usage\
&nbsp;&nbsp;&nbsp;&nbsp;--list                   list devices\
&nbsp;&nbsp;&nbsp;&nbsp;--type                   types of devices to find : playback, capture, playback | capture, playback& capture\
&nbsp;&nbsp;&nbsp;&nbsp;--lang                   language to be used

Example:\
&nbsp;&nbsp;&nbsp;&nbsp;find_devices --name "USB Audio" --desc "Texas Instruments" --no-verbose\
&nbsp;&nbsp;&nbsp;&nbsp;find_devices --list\
&nbsp;&nbsp;&nbsp;&nbsp;find_devices --list --type playback | capture\
&nbsp;&nbsp;&nbsp;&nbsp;find_devices --help\
&nbsp;&nbsp;&nbsp;&nbsp;find_devices --list --json --file out.json

## Practical Examples

### Signalink
### Digirig
### Sabrent USB sound adapter
### FTDI USB to TTL cable

## Strategies for finding audio devices and serial ports 

### Only one USB sound card in the system with VOX, ex: Signalink

- find audio device by name and description

### Two USB sound card in the system with VOX, but with different audio CODECs models or CODEC manufacturers, ex: Signalink and Digilink Nano

- find audio device by name and description

### Only one USB sound card in the system without VOX and without Serial PTT; One FTDI USB serial for PTT via RTS

- find audio device by name and description
- find serial port by name and description, or by serial number

### Only one USB sound card in the system with serial PPT, ex: Digirig

#### Strategy 1

- find audio device by name and description
- find serial port by name and description

#### Strategy 2

- find audio device by name and description
- find serial port by topology sibling

#### Strategy 3

- find serial port by name and description, or serial number
- find audio device by topology sibling

### Two or more USB sound card in the system with VOX, of the same model and manufacturer, ex: two or more Signalink

- find audio device by bus number and device number

### Two or more USB sound card in the system without VOX, of the same model and manufacturer, ex: two or more Digirig

- find audio device by bus number and device number
- find serial port by topology sibling

