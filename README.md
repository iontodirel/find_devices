 # find_devices

Audio device and serial ports search utility.

Use this utility to search for audio devices and serial ports.

Functionally focused on ham radio use, and integration with Direwolf.

## Basic example usage

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

### Example for finding a Digirig

`./find_devices --audio-begin --desc "C-Media" --audio-end --port-begin --desc "CP2102N" --port-end --search-mode port-siblings --json`

### JSON parsing example with jq

Install `jq`.

Run `find_devices`:

`./find_devices --name "USB Audio" --desc "Texas Instruments" --type 'capture&playback' --json --no-verbose | jq -r ".devices[0].plughw_id" ` \
plughw:1,0

## Building

Install the dependecies listed in `install_dependencies.sh`.

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

This project uses `libudev` and `libsound2` as development library dependencies.

This project is using `CMake`, and has been tested with the GCC compiler.

### Development

You can use `Visual Studio` or `VSCode` for remote Linux development.

My setup include a Linux machine running Ubuntu 22.03 Desktop. I do the development in Windows in VSCode or Visual Studio.

#### VSCode

I have CMake Tools, Cpp Tools, and Remote - SSH tools installed. They provide the developer experience for editing, building and debugging.

## Limitations

- Only Linux is currently supported.
- Only USB serial ports are currently supported.
- Limited support for built in sound cards, like PCI based sound cards.
- Requires a Linux kernel with udev support.

## Motivation

- Easy and quick enumeration of audio devices and serial ports
- Programmable support, with easy integration and no scripting required
  - Output from the program can directly be consumed by other programs
- Correlation between audio devices and serial ports
- Repeatable results, across system restarts, or when using generic sound cards.
- Easily find USB sound cards on the same hub as a USB serial port.


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

## Examples

### Signalink
### Digirig

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

