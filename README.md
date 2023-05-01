 # find_devices

Alsa audio device and serial ports finding utility.

Use this utility to list and filter audio devices and serial ports.

## Example usage

### Comparison with Alsa

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

### find_devices example of a playback and record device, by name and description

`./find_devices --name "USB Audio" --desc "Texas Instruments" --type 'capture&playback' --json --no-verbose`\
{\
&nbsp;&nbsp;&nbsp;&nbsp;"devices": [ \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;{ \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"card_id": 1, \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"device_id": 0, \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"plughw_id": "plughw:1,0",  \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"hw_id": "hw:1,0",  \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"name": "USB AUDIO  CODEC",  \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"description": "BurrBrown from Texas Instruments USB AUDIO  CODEC at usb-0000:01:00.0-1.1, full", \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"type": "capture|playback" \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;} \
&nbsp;&nbsp;&nbsp;&nbsp;] \
}

### find_devices example of no devices found

`./find_devices --name "none" --type 'capture&playback' --json --no-verbose`\
{\
&nbsp;&nbsp;&nbsp;&nbsp;"devices": [ \
&nbsp;&nbsp;&nbsp;&nbsp;] \
}

### find_devices example with jq

`./find_devices --name "USB Audio" --desc "Texas Instruments" --type 'capture&playback' --json --no-verbose | jq -r ".devices[0].plughw_id" ` \
plughw:1,0

## Building

Install the dependecies listed in `install_dependencies.sh`.

Clone the repo with `git clone https://github.com/iontodirel/find_devices.git`.

Generate your build scripts with CMake and build:

~~~~
cd find_devices/
mkdir out
cd out
cmake ..
make
~~~~

### Dependencies

This project uses `libudev, `libsound2` as libarary dependencies.

This project is using `CMake`, and has been tested with the GCC compiler.

## Development

You can using `Visual Studio` or `VSCode` for remote Linux development. My setup include a Linux machine running Ubuntu 22.03 Desktop, and I SSH into it.

## Limitations

- Only Linux is supported.
- Only USB serial ports are currently supported.
- Limited support for built in sound cards, like PCI based ones.
- Currently assumes you use a Linux kernel with udev support.

## Motivation

- Easy and fast enumeration of ALSA devices and serial ports

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



