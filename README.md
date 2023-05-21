 # find_devices

Audio device and serial ports search utility. Use this utility to search for audio devices and serial ports.

The functionally of this utility is focused for ham radio use, to help with integration and automation with Direwolf. But it can be useful as an one in all utility.

`./find_devices -p -i all --audio.desc Texas --port.desc cp21`

![image](https://github.com/iontodirel/find_devices/assets/30967482/16e63b8e-40b7-43b4-9cbe-212f95ac3c2d)

## Motivation

- Easy and quick enumeration of audio devices and serial ports within one tool
- Programmable support, with easy integration and no scripting or text processing required
  - Do not need to parse and manipulate text from logs, or parse output from `aplay` and `arecord`
  - Output from the program can directly be consumed by other programs
  - JSON output writing and printing for easy programmability, with tools like `jq`
  - Project is well structured and hackable for future modifications
- Correlation between audio devices and serial ports
  - A big problem for hams is finding a serial port on the same USB hub as the USB sound card
  - Easily find USB sound cards on the same hub as a USB serial port.  
- Repeatable results, across system restarts, or when using generic sound cards with identical USB descriptors.
  - A big problem for hams is using multiple sound cards that have the same USB descriptors
  - This tool aims at reliably finding devices and addresing them individually, even if the USB descriptors are all the same
- No system modifications requirements, no need for udev rules

## Table of contents

- [Motivation](#motivation)
- [Limitations](#limitations)
- [Basic example usage](#basic-example-usage)
  - [Retrieving audio capture and playback devices in JSON format](#retrieving-audio-capture-and-playback-devices-in-json-format)
  - [Print sound cards and serial ports to stdout](#print-sound-cards-and-serial-ports-to-stdout-find_devices)
  - [Print detailed information about each device](#print-detailed-information-about-each-device-find_devices--p)
- [JSON parsing example with jq](#json-parsing-example-with-jq)
- [Building](#building)
  - [Dependencies](#dependencies)
  - [Development](#development)
  - [Github Actions](#github-actions)
  - [Container](#container)
- [Practical Examples](#practical-examples)
- [Strategies for finding devices](#strategies-for-finding-devices)

## Limitations

- Only Linux is currently supported.
- Only USB serial ports are currently supported.
- Limited support for built in sound cards, like PCI based sound cards.
- Requires a Linux kernel with udev support.

## Basic example usage

### Retrieving audio capture and playback devices in JSON format

`./find_devices --audio.type 'capture&playback' -j`\
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

### Print sound cards and serial ports to stdout: `./find_devices`

![image](https://github.com/iontodirel/find_devices/assets/30967482/36f088c3-a332-4329-aaff-eeb28c45b7ee)

### Print detailed information about each device: `./find_devices -p`

![image](https://github.com/iontodirel/find_devices/assets/30967482/7c017b89-581f-465d-89ef-8966e4a327f9)

### Scripting example

Install `jq`.

Run `./find_devices -j | jq -r ".audio_devices[0].plughw_id" ` 

The program will print `plughw:1,0`

For a more complex scripting example look at `examples/find_devices_scripting_example.sh`

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

This project also utilizes the `fmt`, `cxxopts` and `nlohmann::json` libraries, which are automatically installed in source form by the CMake build.

### Development

You can use `Visual Studio` or `VSCode` for remote Linux development.

My setup include a Linux machine running Ubuntu 22.03 Desktop. I do the development nn Windows in VSCode or Visual Studio.

In VSCode have CMake Tools, Cpp Tools, and Remote - SSH tools installed. They provide the developer experience for editing, building and debugging this project.

### Github Actions

A build action automatically builds the project code commits. This makes sure the project builds successfully and the build is well maintained.

### Container

A Docker file is provided which builds `find_devices`.

You can build the container using `docker build -t find_devices .`

You can inspect into the container using `docker container run --interactive --tty --entrypoint /bin/sh find_devices`

To copy the executable built in the container from the container:

~~~~
docker run --name find_devices find_devices
docker cp find_devices:/find_devices/find_devices .
~~~~

## Strategies for finding devices

Always use device properties that are unique and uniquely identify your device. Device `names` and `descriptions` are reliable ways to find devices, but they fall apart for example for sound cards, if you have more than one of the same sound card attached to your system. This is because the USB CODECs powering these sound cards have  identical USB descriptors. If you have two Digirig for example, you cannot use their name or description to find the one you want.

The `hardware path` for USB sound cards or serial ports is a reliable and portable way to uniquely identify devices, as long as the same physical USB port is used to attach them. This is the only strategy that works for multiple Signalink devices, which don't have a serial port, and only a Texas Instruments USB CODEC.

Original FTDI devices typically have a unique `serial number`, use it to reliably find USB serial ports. FTDI clones do not however have unique serial numbers. Use the `hardware path` for FTDI clones. 

Devices like the Digirig have a hub internally, and they expose both a serial port used for PTT, and a USB CODEC, both on the same hub. Find the Digirig USB serial port, find its `serial number`, and then find the sibling USB sound card using the `-s port-siblings` command line option. If for whatever reason the serial number is not unique, you can use the same port-siblings approach, but use te `hardware path` to find the serial port as a fallback. As only one of the two hub attached devices need to be found.

## Practical Examples

### Digirig

Digirig devices typically have a unique serial number for their serial port.

Below is an example where given two or more Digirig devices attached to the system, we find the one with the serial port serial number `e804c4c07cc3ec119e57a4f2d297222e`, then find the sound card associated with it:

`./find_devices --port.serial e804c4c07cc3ec119e57a4f2d297222e --ignore-config -p -i all -s port-siblings`

The output of this command will be pretty stout text containing the Digirig serial port and sound card, as well as a JSON file containing all the same data.

**Note** that `--ignore-config`, `-p`, and  `-i all` are optional. 

This command could be used to find the serial ports that typically belong to Digirig devices:

`./find_devices --port.desc CP21 --ignore-config -p --disable-file-write -i ports`

**Note** that `--ignore-config`, `-p`, and  `--disable-file-write` are optional. 

If you only have one Digirig, you could simple run:

`./find_devices --port.desc CP2102N -s port-sibling`

### Signalink
### Sabrent USB sound adapter
### FTDI USB to TTL cable

Original FTDI devices typically have a unique serial number.
