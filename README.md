 # find_devices

Alsa device finding utility

## Example usage and comparison to Alsa

### Alsa examples

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



