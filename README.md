 # find_devices

Audio device and serial ports search utility. Use this utility to find audio devices and/or serial ports.

The functionally of this utility is tailored for amateur radio use. It can help with integration and automation with Direwolf and other related software. Aditionally this utility can be useful as a one-in-all tool.

`./find_devices -p -i all --audio.desc Texas --port.desc cp21`

![image](https://github.com/iontodirel/find_devices/assets/30967482/16e63b8e-40b7-43b4-9cbe-212f95ac3c2d)

## Capabilities

- Enumerating sound cards
- Enumerating serial ports
- Audio volume enumeration and control
- Mapping sound ports to serial ports (and vice versa)
- Printing to stdout and to JSON
- Search from command line, or via a JSON configuration file

## Table of contents

- [Capabilities](#capabilities)
- [Motivation](#motivation)
- [Limitations](#limitations)
- [Basic example usage](#basic-example-usage)
  - [Retrieving audio capture and playback devices in JSON format](#retrieving-audio-capture-and-playback-devices-in-json-format)
  - [Print sound cards and serial ports to stdout](#print-sound-cards-and-serial-ports-to-stdout-find_devices)
  - [Print detailed information about each device](#print-detailed-information-about-each-device-find_devices--p)
  - [Volume Control](#volume-control)
  - [Scripting example](#scripting-example)
- [Building](#building)
  - [Dependencies](#dependencies)
  - [Development](#development)
    - [Specifying Command Line Arguments When Debugging in VSCode](#specifying-command-line-arguments-when-debugging-in-vscode)
    - [API Examples](#api-examples)
  - [Github Actions](#github-actions)
  - [Container](#container)
- [Strategies for finding devices](#strategies-for-finding-devices)
- [Practical Examples](#practical-examples)
  - [Digirig](#digirig)
  - [Signalink](#signalink)
  - [FTDI](#ftdi-usb-to-ttl-cable-and-ftdi-usb-serial-devices)
  - [u-blox](#u-blox-gps-devices)
  - [Microsoft Surface Go](#microsoft-surface-go-2)
  - [DigiLink Nano](#digilink-nano)
  - [Icom ID-52](#icom-id-52)
  - [Icom IC-705](#icom-ic-705)
  - [Kenwood TH-D74](#kenwood-th-d74)
  - [PicoAPRS v4](#picoaprs-v4)

## Motivation

- Easy and quick enumeration of audio devices and serial ports within one tool
- Programmatic support, with easy integration, and no text processing
  - Do not need to parse and manipulate text from logs, or parse text output from `aplay` and `arecord`
  - Output from this utility can directly be consumed by other programs (ex: via jq)
  - Structured output via JSON output. JSON output writing and printing for easy programmability, with tools like `jq`
  - Project is structured in modular fashion, with pieces that can be easily reused
    - Hackable for future or further modifications (ex: libusb support)
    - No coupling between utlity classes
    - Command line parsing is not coupled to the audio and udev utility classes
- Mapping between audio devices and serial ports
  - A big problem for hams can be finding a serial port that's on the same USB hub as an USB sound card
    - Easily find USB sound cards on the same hub as a USB serial port.  
- Repeatable results, across system restarts, or when using generic sound cards with identical USB descriptors.
  - A big problem for hams can be using multiple sound cards on the same compuper, which have the same USB descriptors
  - This tool aims at reliably and repeatedly finding devices and addresing them individually, even if the USB descriptors are all the same
- No system modifications requirements, no need for udev rules, no need for root access

## Limitations

- Only Linux is currently supported.
- Only USB serial ports are currently supported.
- Limited support for built in sound cards, like PCI or PCH based sound cards.
- Requires a Linux kernel with udev support.

## Basic example usage

### Retrieving audio capture and playback devices in JSON format

`./find_devices --audio.type 'capture&playback' -j`

```json
{
    "audio_devices": [
        {
            "card_id": "0",
            "device_id": "0",
            "plughw_id": "plughw:0,0",
            "hw_id": "hw:0,0",
            "name": "USB PnP Sound Device",
            "description": "C-Media Electronics Inc. USB PnP Sound Device at usb-0000:00:14.0-4.1.1, full s",
            "type": "capture&playback",
            "controls": [
                {
                    "name": "Speaker",
                    "channels": [
                        {
                            "name": "Front Left",
                            "type": "playback",
                            "volume": "100",
                            "channel": "front_left"
                        },
                        {
                            "name": "Front Right",
                            "type": "playback",
                            "volume": "75",
                            "channel": "front_right"
                        }
                    ]
                }
            ],
            "bus_number": "1",
            "device_number": "7",
            "id_product": "013c",
            "id_vendor": "0d8c",
            "device_manufacturer": "C-Media Electronics Inc.      ",
            "path": "/sys/devices/pci0000:00/0000:00:14.0/usb1/1-4/1-4.1/1-4.1.1/1-4.1.1:1.0/sound/card0",
            "hw_path": "/sys/devices/pci0000:00/0000:00:14.0/usb1/1-4/1-4.1/1-4.1.1",
            "product": "USB PnP Sound Device",
            "topology_depth": "4"
        }
    ]
}
```

### Print sound cards and serial ports to stdout: `./find_devices`

![image](https://github.com/iontodirel/find_devices/assets/30967482/36f088c3-a332-4329-aaff-eeb28c45b7ee)

### Print detailed information about each device: `./find_devices -p`

![image](https://github.com/iontodirel/find_devices/assets/30967482/7c017b89-581f-465d-89ef-8966e4a327f9)

### Volume Control

This utility is capable of enumerating and changing an audio device's volume controls.

The volume controls are printed in stdout in a compact format:

![image](https://github.com/iontodirel/find_devices/assets/30967482/cf9f1dac-040b-447b-b340-06b2ac25bf58)

The volume controls are also written to JSON:

```json
"controls": [
   {
       "name": "Master",
       "channels": [
           {
               "name": "Front Left",
               "type": "playback",
               "volume": "88",
               "channel": "front_left"
           }
       ]
   },
   {
       "name": "Headphone",
       "channels": [
           {
               "name": "Front Left",
               "type": "playback",
               "volume": "0",
               "channel": "front_left"
           },
           {
               "name": "Front Right",
               "type": "playback",
               "volume": "0",
               "channel": "front_right"
            }
        ]
    }
]
```

The bash script in this repositry has an example about retrieving a volume control and setting it using bash here: examples/find_devices_scripting_example.sh

The example uses find_devices to write the volume controls to JSON. For each volume control, we print the name, type and value, and we set it to 100%.

To change the volume via command line: `find_devices --audio.control Speakers --audio.channels="Front Left,Front Center" --audio.volume 60 --audio.channel-type=capture`

You can also change the volume of all capture channels without specifying the control or channel name: `find_devices --audio.volume 100 --audio.channel-type=capture`

To change a volume control using amixer: `amixer -c 0 sset Speaker 100%`

### Scripting example

Install `jq`.

Run `./find_devices -j | jq -r ".audio_devices[0].plughw_id" ` 

The program will print `plughw:1,0`

To print the first sound card followed by the first serial port name: `./find_devices -j | jq -r '.audio_devices[0].plughw_id // "",.serial_ports[0].name // ""'`

The program will print:

~~~~
plughw:0,3
/dev/ttyUSB3
~~~~

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

### Runtime Dependencies

This project uses `libudev` and `libsound2`, please install them with your system package manager or by running `install_dependencies.sh` on a Debian system.

### Development

You can use `Visual Studio` or `VSCode` for remote or native Linux development.

My deveopment setup include a Linux machine running Ubuntu 22.03 Desktop. I do the development on a Windows Laptop in VSCode or Visual Studio. With VSCode or Visual Studio connecting to a Linux system. 

In VSCode, I have the following extensions installed: CMake Tools, Cpp Tools, and Remote - SSH. They provide the developer experience for editing, building and debugging this project.

#### Specifying command line arguments when debugging in VSCode

An example of what you could specify in the .vscode/settings.json file:

```json
"cmake.debugConfig": {
    "args": [
        "-c",
        "/home/iontodirel/ham_docker_container/digirig_config.json"
    ]
}
```

#### API Examples

Audio API:

```cpp
// get all audio devices
std::vector<audio_device_info> devices = get_audio_devices();

// get the volume controls of the first audio device
audio_device_volume_info volume;
try_get_audio_device_volume(devices[0], volume);
```

Serial Ports API:

```cpp
// get all serial ports
std::vector<serial_port> ports = get_serial_ports();
```

Mapping API:

```cpp
// get all serial ports
std::vector<serial_port> ports = get_serial_ports();

// get hardware device description for serial port 1
device_description serial_port_description;
try_get_device_description(ports[1], serial_port_description);

// get audio device hardware description on the same USB hub as the serial port
std::vector<device_description> device_descriptions = get_sibling_audio_devices(serial_port_description);

// get the audio device
std::vector<audio_device_info> devices = get_audio_devices(device_descriptions[0]);
```

### Github Actions

A build action automatically builds the project code commits. This makes sure the project builds successfully and the build is well maintained.

### Container

A Docker file is provided which builds `find_devices`.

You can build the container using `docker build -t find_devices .`

You can inspect into the container using `docker container run --interactive --tty --entrypoint /bin/sh find_devices`

To copy the executable built in the container from the container:

```sh
docker run --name find_devices find_devices
docker cp find_devices:/find_devices/find_devices .
```

## Strategies for finding devices

Always use device properties that are unique and uniquely identify your device. Device `names` and `descriptions` are reliable ways to find devices, but they fall apart for example for sound cards, if you have more than one of the same sound card attached to your system. This is because the USB CODECs powering these sound cards have  identical USB descriptors. If you have two Digirig for example, you cannot use their name or description to find the one you want.

The `hardware path` for USB sound cards or serial ports is a reliable and portable way to uniquely identify devices, as long as the same physical USB port is used to attach them. This is the only strategy that works for multiple Signalink devices, which don't have a serial port, and only a Texas Instruments USB CODEC.

Authentic FTDI devices typically have a unique `serial number`, use it to reliably find USB serial ports. FTDI clones do not typically have unique serial numbers, but you might be lucky to have one. Use the `hardware path` for FTDI clones. 

Devices like the Digirig have a hub internally, and they expose both a serial port used for PTT, and a USB CODEC, both on the same hub. Find the Digirig USB serial port, find its `serial number`, and then find the sibling USB sound card using the `-s port-siblings` command line option. If for whatever reason the serial number is not unique, you can use the same port-siblings approach, but use te `hardware path` to find the serial port as a fallback. As only one of the two hub attached devices need to be found.

## Practical Examples

All the examples shared here were created using real hardware devices.

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

![image](https://github.com/iontodirel/find_devices/assets/30967482/36efb00d-5000-46f8-9924-77e3492bdb40)


### Signalink

If you only have one Signalink, run `./find_devices -p --audio.desc "Texas Instruments" -i audio`

**Note** that `-p` and `-i` are optional.

You might have to adjust your query, as there might be variations in the USB descriptor and CODED used. I've seen some descriptors content having multiple spaces like `USB AUDIO  CODEC`, note the extra space between AUDIO and CODEC.

If you only have one Signalink attached, the audio device `description` is a robust and portable way to find your Signalink.

If you have more than one Signalink in your system, use the `hardware path`:

`./find_devices --audio.path /sys/devices/pci0000:00/0000:00:14.0/usb1/1-4`

Find the hardware path by running `find_devices` with the `-p` option. If you find trouble finding your device (ex: you have 10 Signalink devices), remove all devices and only attach the you are querying about on the USB port you intend to use. Other techniques could include playing tones to your Signalink selectively until you find the right one. You could also find the hardware path by connecting a different device on the port you need to get the path for, if that makes it easier to find the port. You only have to do it once.

The `hardware path` is going to remain the same as long so you do not change the physical USB port where you insert your Signalink.

![image](https://github.com/iontodirel/find_devices/assets/30967482/feb768b4-9f5b-4539-b4a2-52c15feaf18a)


### Sabrent USB sound adapter

Follow the same approach as with the Signalink. USB descriptors on Sabrent devices are not unique, if you have more than one attached, use the `hardware path`. 

In addition, configure a query to find the serial port of your choice to drive the PTT. Follow the same approach as for FTDI devices below, and specify the search mode to independent with `-s independent`.

Example for one Sabrent device and one FTDI serial port:

`./find_devices -s independent --audio.desc C-Media --port.serial A50285BI`

![image](https://github.com/iontodirel/find_devices/assets/30967482/8a2d5b24-9cd5-4fe6-b5ff-1ab7bb6dc643)


### FTDI USB to TTL cable and FTDI USB serial devices

Authentic FTDI devices typically have a unique serial number, and can be found by serial number.

If you only have one FTDI device, you could search using the `manufacturer` name: `./find_devices -i ports -p --port.mfn FTDI`

If you have multiple FTDI devices, you can search by serial number: `./find_devices -i ports -p --port.serial A9JF6F84`

If the serial numbers are not unique across your serial port USB devices, use the `hardware path`: `./find_devices -i ports -p --port.path /sys/devices/pci0000:00/0000:00:14.0/usb1/1-3/1-3.2`

![image](https://github.com/iontodirel/find_devices/assets/30967482/5e7e6f31-0220-41f6-9260-7fc4ab180a22)

### u-blox GPS devices

You can find them just like any other serial port devices, here is an example if you have one attached: `./find_devices -i ports -p --port.mfn u-blox`

![image](https://github.com/iontodirel/find_devices/assets/30967482/dbddfaab-3a15-4d72-b945-d649463ebed5)


### Microsoft Surface Go 2

On the Surface and other computers (running Linux) with built in audio, the audio device is typically built into the PCH, and is not a USB attached sound card.

The Surface Go 2 computer is running Ubuntu 22.03.

Use a filter on the `stream name` to find it: `./find_devices -i audio -p --audio.stream_name ALC298`

![image](https://github.com/iontodirel/find_devices/assets/30967482/feefd269-33ea-43d2-a0a1-f815941b63ee)

### DigiLink Nano

The DigiLink Nano uses a Texas Instruments CODEC (PCM2912A) just as the Signalink. There are no unique USB descriptors to uniquely identify multiple DigiLink Nano devices connected to the same system.

Just like the Signalink, if you have only one DigiLink Nano soundcard connected, and no other USB soundcards that utilize Texas Instruments CODECs, then you can use the `name` or `description` properties like so:

`./find_devices -p --audio.desc "TI USB audio CODEC" -i audio` \
`./find_devices -p --audio.name "USB audio CODEC" -i audio`

The name and description are a little bit different across different Texas Instruments USB Codec parts, but I would not rely on this unless you have tested it yourself.

If you want to uniquely identify one of many DigiLink Nano devices use the `hardware path`:

`./find_devices -p --audio.path /sys/devices/pci0000:00/0000:00:14.0/usb1/1-1`

Because the Signalink uses a different TI CODEC part number, with different idProduct values and slightly different USB descriptors, you should be able to use a DigiLink Nano and a Signalink in the system, and uniquely identify each just by `name` or `description`. 

![image](https://github.com/iontodirel/find_devices/assets/30967482/40b4e3cd-1bcc-4dff-8836-1b7c8032aaae)

### Icom ID-52

The Icom ID-52 exposes one serial port when connected to a computer via the micro-USB port. The manufacturer and description in the USB descriptor are set to "Icom" and "ID-52". The serial number is also set, and I believe it to be unique. They match the device serial number written on the back, behind the battery.

Here is an example to find one: `./find_devices -i ports -p --port.serial "15002168"`

![image](https://github.com/iontodirel/find_devices/assets/30967482/9edb2efa-42d9-4e33-8813-e91d3c86a686)

### Icom IC-705

The Icom IC-705 exposes three USB devices when connected to a computer via the micro-USB port. One USB device is a soundcard, and two USB devices are serial ports.

The two serial ports correspond to USB A and USB B, as referenced throught the IC-705 menus. Both serial ports have the same unique serial number, but they cannot be unique identified other than throught the `hardware path`. The serial number match the device serial number written on the back, behind the battery.

The USB soundcard is implemented using a Texas Instruments CODEC, typically PCM2901 (some revisions might use a different one). The USB soundcard and the serial ports are on the same USB hub, as such the sound card can be found robustly using the serial port's serial number, even when multiple IC-705 are connected to the same computer.

To find the sound card, first find the serial port's serial number by finding all the 705s: `./find_devices -p -i ports --port.desc "IC-705"`

After you get the serial number for the device you want, find the sound card: `./find_devices --port.serial 12007514 -p -i audio -s port-siblings`

If you have to find USB A or USB B, you can use the `hardware path`.

![image](https://github.com/iontodirel/find_devices/assets/30967482/c2f5edbd-78dc-4a30-bbd1-015706e3dddf)

### Kenwood TH-D74

The Kenwood TH-D74 exposes two USB devices when connected to a computer via the micro-USB port. One USB device is a soundcard, and the other is a serial port.

The sound card name is "TH-D74" and description contains "JVC KENWOOD TH-D74", there are no unique identifiers.

The serial port manufacturer is "JVC KENWOOD" and description is "TH-D74", there are no unique identifiers.

If you want to uniquely identify one of many TH-D74 devices use the `hardware path`: 

`./find_devices -p --audio.path /sys/devices/pci0000:00/0000:00:14.0/usb1/1-1 --port.path /sys/devices/pci0000:00/0000:00:14.0/usb1/1-1`

![image](https://github.com/iontodirel/find_devices/assets/30967482/3e7b5d6b-7e47-4d93-ac55-d911321d3c37)

### PicoAPRS v4

This device exposes a serial port over its USBC-C port, when it is in KISS TNC mode. The implementation seems to be based on Silicon Labs CP2102N, and it has a unique serial number.

Find it using: `./find_devices -p -i ports --port.serial e044f8266decec1193fbb20ead51a8b2`

![image](https://github.com/iontodirel/find_devices/assets/30967482/2fc5cc0f-d3d8-457d-8685-5b48e92c832d)
