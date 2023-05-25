 #!/bin/sh

# **************************************************************** #
# find_devices - Audio device and serial ports search utility      #
# Version 0.1.0                                                    #
# https://github.com/iontodirel/find_devices                       #
# Copyright (c) 2023 Ion Todirel                                   #
# **************************************************************** #

# You can set FIND_DEVICES and other variables in the shell with "export FIND_DEVICES=/path/to/executable"

# NOTE: Please modify the path to 'find_devices' as appropriate by updating FIND_DEVICES
: "${FIND_DEVICES:=/usr/bin/find_devices}"
# Generic configuration in the same directory as the script
: "${CONFIG_JSON:=generic_config.json}"
# Can simply remain the same
: "${OUT_JSON:=output.json}"
# Update with your own direwolf.conf file and location
# this file is located in the same directory as this script
: "${DIREWOLF_CONFIG_FILE:=direwolf.conf}"

# Check that the find_devices utility is found
if ! command -v "$FIND_DEVICES" >/dev/null 2>&1; then
    echo "Executable" \"$FIND_DEVICES\"" not found"
    exit 1
fi

# Find devices
if ! $FIND_DEVICES -c $CONFIG_JSON -o $OUT_JSON --no-stdout; then
    echo "Failed to find devices"
    exit 1
fi

# Get counts and names
audio_devices_count=$(jq ".audio_devices | length" $OUT_JSON)
serial_ports_count=$(jq ".serial_ports | length" $OUT_JSON)
# Pick the first sound card and serial port
# Adjust your configuration to alwats find one device
audio_device=$(jq -r .audio_devices[0].plughw_id $OUT_JSON)
serial_port=$(jq -r .serial_ports[0].name $OUT_JSON)

# Optional to convert the null values to empty
# could also be done with jq, but null is nicer to work with that empty
if [[ -z $audio_device ]]; then
    audio_device=""
fi
if [[ -z $serial_port ]]; then
    serial_port=""
fi

echo "Audio devices count: \"$audio_devices_count\""
echo "Serial ports count: \"$serial_ports_count\""
echo "Audio device: \"$audio_device\""
echo "Serial port: \"$serial_port\""

# Return if no soundcards and serial ports were found
if [ $audio_devices_count -eq 0 ] || [ $serial_ports_count -eq 0 ]; then
     echo "No audio devices and serial ports found, expected at least one soundcard and at least one serial port"
     exit 1
fi

# Check counts
# Update as appropriate
# Uncomment or comment next lines after as you are writing your configuration
# to find exactly one devices
if [ $audio_devices_count -ne 1 ]; then
    echo "Audio devices not equal to 1"
    exit 1
fi
if [ $serial_ports_count -ne 1 ]; then
    echo "Serial ports not equal to 1"
    exit 1
fi

# Use the ALSA sound card id for something
echo "Using: $audio_device"

# Replacing "ADEVICE plughw:0,0" with the device we have found in direwolf.conf
echo "Updating the soundcard in the \"direwolf.conf\" file with \"$audio_device\""
sed -i "s/ADEVICE.*/ADEVICE $audio_device/" $DIREWOLF_CONFIG_FILE

exit 0