// find_devices.cpp
// Audio device and serial ports finding utility.
//
// MIT License
//
// Copyright (c) 2022 Ion Todirel
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "find_devices.hpp"

#include <functional>

#include <alsa/asoundlib.h>
#include <libudev.h>
#include <fmt/format.h>

// **************************************************************** //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
// UTILITIES                                                        //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
// **************************************************************** //

void insert_tabs(std::string& s, int tabs, int tab_spaces = 4);

void insert_tabs(std::string& s, int tabs, int tab_spaces)
{
    std::string padded(tabs * tab_spaces, ' ');

    // Efficiency is not eally important here, but one 
    // possible improvement is to use s.find('\n', pos)
    // and insert the new lines in one go

    std::string result;
    std::istringstream iss(s);
    std::string line;
    while (std::getline(iss, line))
    {
       result += padded + line + "\n";
    }

    if (result.length() > 0)
        result.erase(result.length() - 1);

    s = result;
}

// **************************************************************** //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
// AUDIO DEVICE                                                     //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
// **************************************************************** //

audio_device_type operator|(const audio_device_type& l, const audio_device_type& r);
audio_device_type operator&(const audio_device_type& l, const audio_device_type& r);
bool enum_device_type_has_flag(const audio_device_type& deviceType, const audio_device_type& flag);
std::string to_string(const audio_device_type& deviceType);
std::string to_string(const audio_device_info& d);
std::string to_json(const std::vector<audio_device_info>& devices);
std::string to_json(const audio_device_info& d, bool wrapping_object, int tabs);
std::vector<audio_device_info> get_audio_devices();
std::vector<audio_device_info> get_audio_devices(int card_id);
bool try_get_audio_device(int card_id, snd_ctl_t*& ctl_handle);
bool try_get_audio_device(int card_id, int device_id, snd_ctl_t*& ctl_handle, snd_pcm_info_t*& pcm_info);
bool try_get_audio_device(int card_id, int device_id, snd_ctl_t* ctl_handle, audio_device_info& device);
bool can_use_audio_device(const audio_device_info& device, snd_pcm_stream_t mode);
bool can_use_audio_device(const audio_device_info& device);

audio_device_type operator|(const audio_device_type& l, const audio_device_type& r)
{
    return (audio_device_type)((int)l | (int)r);
}

audio_device_type operator&(const audio_device_type& l, const audio_device_type& r)
{
    return (audio_device_type)((int)l & (int)r);
}

bool enum_device_type_has_flag(const audio_device_type& deviceType, const audio_device_type& flag)
{
    return (deviceType & flag) != (audio_device_type)0;
}

std::string to_string(const audio_device_type& deviceType)
{
    if (deviceType == audio_device_type::capture)
        return std::string("capture");
    else if (deviceType == audio_device_type::playback)
        return std::string("playback");
    else if (deviceType == audio_device_type::uknown)
        return std::string("unknown");
    else if ((deviceType & audio_device_type::capture) != (audio_device_type)0 && (deviceType & audio_device_type::playback) != (audio_device_type)0)
        return std::string("capture&playback");
    return std::string("");
}

std::string to_string(const audio_device_info& d)
{
    return std::string("card id: '") + std::to_string(d.card_id) + "', " +
        "device id: '" + std::to_string(d.device_id) +
        "', name: '" + d.name +
        "', desc: '" + d.description +
        "', type: '" + to_string(d.type) + "'";
}

std::string to_json(const std::vector<audio_device_info>& devices)
{
    std::string s;
    s.append("{\n");
    s.append("    \"devices\": [\n");
    for (size_t i = 0; i < devices.size(); i++)
    {
        const audio_device_info& d = devices[i];
        s.append(to_json(d, 2));
        if ((i + 1) < devices.size())
        {
            s.append(",");
        }
        s.append("\n");
    }
    s.append("    ]\n");
    s.append("}\n");
    return s;
}

std::string to_json(const audio_device_info& d, bool wrapping_object, int tabs)
{
    std::string s;
    if (wrapping_object)
    {
        s.append("{\n");
    }
    s.append("    \"card_id\": \"" + std::to_string(d.card_id) + "\",\n");
    s.append("    \"device_id\": \"" + std::to_string(d.device_id) + "\",\n");
    s.append("    \"plughw_id\": \"" + d.plughw_id + "\",\n");
    s.append("    \"hw_id\": \"" + d.hw_id + "\",\n");
    s.append("    \"name\": \"" + d.name + "\",\n");
    s.append("    \"description\": \"" + d.description + "\",\n");
    s.append("    \"type\": \"" + to_string(d.type) + "\"");
    if (wrapping_object)
    {
        s.append("\n");
        s.append("}");
    }
    insert_tabs(s, tabs);
    return s;
}

std::vector<audio_device_info> get_audio_devices()
{
    std::vector<audio_device_info> devices;

    int card_id = -1;
    int err = -1;

    // ALSA sample code: https://github.com/bear24rw/alsa-utils/blob/master/aplay/aplay.c

    // Consider using snd_device_name_hint and snd_device_name_get_hint
    // for more portability, simplicity and support of non PCM devices
    // Can use snd_ctl_rawmidi_next_device for MIDI devices.

    // Iterate over all available sound cards
    while (true)
    {
        err = snd_card_next(&card_id);
        if (err != 0 || card_id < 0)
            break;
        std::vector<audio_device_info> devices_for_card = get_audio_devices(card_id);
        devices.insert(devices.end(), devices_for_card.begin(), devices_for_card.end());
    }

    return devices;
}

std::vector<audio_device_info> get_audio_devices(int card_id)
{
    std::vector<audio_device_info> devices;

    snd_ctl_t* ctl_handle = nullptr;
    int err = -1;

    // ALSA sample code: https://github.com/bear24rw/alsa-utils/blob/master/aplay/aplay.c

    // Consider using snd_device_name_hint and snd_device_name_get_hint
    // for more portability, simplicity and support of non PCM devices
    // Can use snd_ctl_rawmidi_next_device for MIDI devices.

    err = snd_ctl_open(&ctl_handle, fmt::format("hw:{}", card_id).c_str(), 0);
    if (err != 0)
        return devices;;

    // Iterate over all available PCM devices on the current card
    int device_id = -1;
    while (true)
    {
        err = snd_ctl_pcm_next_device(ctl_handle, &device_id);
        if (err != 0)
            continue;
        if (device_id < 0)
            break;

        audio_device_info device;

        if (!try_get_audio_device(card_id, device_id, ctl_handle, device))
            continue;

        devices.push_back(device);
    }

    snd_ctl_close(ctl_handle);    

    return devices;
}

bool try_get_audio_device(int card_id, snd_ctl_t*& ctl_handle)
{
    int err = snd_ctl_open(&ctl_handle, fmt::format("hw:{}", card_id).c_str(), 0);
    if (err != 0)
    {
        return false;
    }
    return true;
}

bool try_get_audio_device(int card_id, int device_id, snd_ctl_t* ctl_handle, audio_device_info& device)
{
    snd_pcm_info_t* pcm_info = nullptr;
    int err = -1;

    snd_pcm_info_alloca(&pcm_info);

    // NOTE: find a better way to identify playback or capture in one go

    snd_pcm_info_set_device(pcm_info, device_id);
    snd_pcm_info_set_subdevice(pcm_info, 0);
    snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_CAPTURE);

    err = snd_ctl_pcm_info(ctl_handle, pcm_info);
    
    if (err < 0)
    {
        snd_pcm_info_set_device(pcm_info, device_id);
        snd_pcm_info_set_subdevice(pcm_info, 0);
        snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_PLAYBACK);

        err = snd_ctl_pcm_info(ctl_handle, pcm_info);
        
        if (err < 0)
        {
            return false;
        }
        else
        {
            device.type = audio_device_type::playback;
        }
    }
    else
    {
        device.type = audio_device_type::capture;
    }

    device.card_id = card_id;
    device.device_id = device_id;
                
    // snd_card_get_name and snd_card_get_longname are implemented here:
    // https://github.com/torvalds/linux/blob/master/sound/usb/card.c
    
    char* name = nullptr;
    err = snd_card_get_name(card_id, &name);
    if (err != 0)
        device.name = "Unknown";
    else
        device.name = name;
    
    device.stream_name = snd_pcm_info_get_name(pcm_info);
    
    char* long_name = nullptr;
    err = snd_card_get_longname(card_id, &long_name);
    if (err != 0)
        device.description = "Unknown";
    else
        device.description = long_name;
    
    device.hw_id = fmt::format("hw:{},{}", device.card_id, device.device_id);
    device.plughw_id = fmt::format("plughw:{},{}", device.card_id, device.device_id);
        
    // Use snd_pcm_info_get_subdevices_avail, snd_pcm_info_set_subdevice, snd_ctl_pcm_info and
    // snd_pcm_info_get_subdevice_name to enumerate all subdevices
    // example here: https://github.com/bear24rw/alsa-utils/blob/master/aplay/aplay.c

    if (device.type == audio_device_type::capture)
    {
        snd_pcm_info_set_device(pcm_info, device_id);
        snd_pcm_info_set_subdevice(pcm_info, 0);
        snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_PLAYBACK);

        err = snd_ctl_pcm_info(ctl_handle, pcm_info);
        
        if (err >= 0)
        {
            device.type = device.type | audio_device_type::playback;
        }
    }

    return true;
}

bool can_use_audio_device(const audio_device_info& device, snd_pcm_stream_t mode)
{
    snd_pcm_t *handle;

    int err = snd_pcm_open(&handle, device.hw_id.c_str(), mode, SND_PCM_NONBLOCK);

    if (err < 0)
    {
        if (err == -EBUSY)
        {
            // Device is busy, cannot open
            return false;
        }
        else if (err == -ENODEV)
        {
            // Device does not exist or is not available
            return false;
        }
        else
        {
            // Other error occurred, handle it as needed
            return false;
        }
    }

    snd_pcm_close(handle);

    return true;
}

bool can_use_audio_device(const audio_device_info& device)
{
    if (device.type == audio_device_type::capture)
    {
        return can_use_audio_device(device, SND_PCM_STREAM_CAPTURE);
    }
    else if (device.type == audio_device_type::playback)
    {
        return can_use_audio_device(device, SND_PCM_STREAM_PLAYBACK);
    }
    else if (enum_device_type_has_flag(device.type, audio_device_type::capture) && 
        enum_device_type_has_flag(device.type, audio_device_type::playback))
    {
        return can_use_audio_device(device, SND_PCM_STREAM_CAPTURE) &&
            can_use_audio_device(device, SND_PCM_STREAM_PLAYBACK);
    }
    return false;
}

// **************************************************************** //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
// SERIAL PORTS                                                     //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
// **************************************************************** //

std::vector<serial_port> get_serial_ports();
std::string to_json(const serial_port& p, bool wrapping_object, int tabs);

std::vector<serial_port> get_serial_ports()
{
    std::vector<serial_port> ports;

    udev* udev = udev_new();
    if (udev == nullptr)
        return ports;

    udev_enumerate* enumerate = udev_enumerate_new(udev);
    if (enumerate == nullptr)
    {
        udev_unref(udev);
        return ports;
    }

    // Only supports USB serial ports
    // Consider using libusb, or USB sysfs in the future
    // Could there be other types of serial ports that are not tty?
    
    udev_enumerate_add_match_subsystem(enumerate, "tty");
    udev_enumerate_add_match_property(enumerate, "ID_BUS", "usb");

    udev_enumerate_scan_devices(enumerate);

    udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    if (devices == nullptr)
    {
        udev_enumerate_unref(enumerate);
        udev_unref(udev);
        return ports;
    }

    udev_list_entry* dev_list_entry;
    udev_list_entry_foreach(dev_list_entry, devices)
    {
        const char* path = udev_list_entry_get_name(dev_list_entry);

        udev_device* dev = udev_device_new_from_syspath(udev, path);
        if (dev == nullptr)
            continue;

        const char* devnode = udev_device_get_devnode(dev);

        udev_device* usb_dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (usb_dev == nullptr)
        {
            udev_device_unref(dev);
            continue;
        }

        // Other properties: udevadm info --attribute-walk --path=/sys/bus/usb-serial/devices/ttyUSB0
        // To inspect the USB tree with libusb: lsusb -t
        // 
        // Source code location: https://github.com/systemd/systemd/blob/main/src/libudev/libudev-device.c

        const char* manufacturer = udev_device_get_sysattr_value(usb_dev, "manufacturer");
        const char* product = udev_device_get_sysattr_value(usb_dev, "product");
        const char* serial = udev_device_get_sysattr_value(usb_dev, "serial");
        
        serial_port port;

        if (manufacturer != nullptr)
            port.manufacturer = manufacturer;
        if (serial != nullptr)
            port.device_serial_number = serial;
        if (product != nullptr)
            port.description = product;
        if (devnode != nullptr)
            port.name = devnode;
        
        ports.push_back(port);

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return ports;
}

std::string to_json(const serial_port& p, bool wrapping_object, int tabs)
{
    std::string s;
    if (wrapping_object)
    {
        s.append("{\n");
    }
    s.append("    \"name\": \"" + p.name + "\",\n");
    s.append("    \"description\": \"" + p.description + "\",\n");
    s.append("    \"manufacturer\": \"" + p.manufacturer + "\",\n");
    s.append("    \"device_serial_number\": \"" + p.device_serial_number + "\"");
    if (wrapping_object)
    {
        s.append("\n");
        s.append("}");
    }
    insert_tabs(s, tabs);
    return s;
}

// **************************************************************** //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
// DEVICES                                                          //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
// **************************************************************** //

bool try_get_device_description(const audio_device_info& d, device_description& desc);
bool try_get_device_description(const serial_port& p, device_description& desc);
bool try_get_device_description(std::function<void(udev_enumerate*)> filter, device_description& desc);
bool try_get_device_description(udev* udev, udev_enumerate* enumerate, udev_device*& device, udev_device*& usb_device, device_description& desc);
bool try_get_device_description(udev_device* device, udev_device*& usb_device, device_description& desc);
void fetch_device_description(udev_device* device, udev_device* usb_device, device_description& desc);
bool try_find_device(udev* udev, udev_enumerate* enumerate, udev_device*& device, udev_device*& usb_device);
bool try_find_device(udev_device* device, udev_device*& usb_device);
int get_topology_depth(udev_device* device);
std::vector<device_description> get_sibling_audio_devices(const device_description& desc);
std::vector<device_description> get_sibling_serial_ports(const device_description& desc);
std::vector<device_description> get_sibling_devices(std::function<void(udev_enumerate*)> filter, const device_description& desc);
std::vector<audio_device_info> get_audio_devices(const device_description& desc);
bool try_get_serial_port(const device_description& desc, serial_port& port);
std::string to_json(const device_description& d, bool wrapping_object, int tabs);

bool try_get_device_description(const audio_device_info& d, device_description& desc)
{
    return try_get_device_description([&d](udev_enumerate* enumerate) {
        udev_enumerate_add_match_subsystem(enumerate, "sound");
        udev_enumerate_add_match_sysname(enumerate, fmt::format("card{}", d.card_id).c_str());
    }, desc);
}

bool try_get_device_description(const serial_port& p, device_description& desc)
{
    return try_get_device_description([&p](udev_enumerate* enumerate) {
        udev_enumerate_add_match_subsystem(enumerate, "tty");
        udev_enumerate_add_match_property(enumerate, "DEVNAME", p.name.c_str());
    }, desc);
}

bool try_get_device_description(std::function<void(udev_enumerate*)> filter, device_description& desc)
{
    bool found = false;

    udev* udev = udev_new();
    if (udev == nullptr)
    {
        return found;
    }

    udev_enumerate* enumerate = udev_enumerate_new(udev);
    if (enumerate == nullptr)
    {
        udev_unref(udev);
        return found;
    }

    filter(enumerate);

    udev_device* device = nullptr;
    udev_device* usb_device = nullptr;
    found = try_get_device_description(udev, enumerate, device, usb_device, desc);

    // NOTE: Should check that the device we found matched the filter? ex: udev_device_get_sysattr_value(dev, "number");
    // Should check that only one enumeration?
    
    udev_device_unref(device);
    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return found;
}

bool try_get_device_description(udev* udev, udev_enumerate* enumerate, udev_device*& device, udev_device*& usb_device, device_description& desc)
{
    if (!try_find_device(udev, enumerate, device, usb_device))
    {
        return false;
    }

    fetch_device_description(device, usb_device, desc);

    return true;
}

bool try_get_device_description(udev_device* device, udev_device*& usb_device, device_description& desc)
{
    if (!try_find_device(device, usb_device))
    {
        return false;
    }

    fetch_device_description(device, usb_device, desc);

    return true;
}

void fetch_device_description(udev_device* device, udev_device* usb_device, device_description& desc)
{
    const char* syspath = udev_device_get_syspath(device);
    const char* usb_syspath = udev_device_get_syspath(usb_device);
    const char* busnum = udev_device_get_sysattr_value(usb_device, "busnum");
    const char* devnum = udev_device_get_sysattr_value(usb_device, "devnum");
    const char* dev_id_vendor = udev_device_get_sysattr_value(usb_device, "idVendor");
    const char* dev_id_product = udev_device_get_sysattr_value(usb_device, "idProduct");
    const char* dev_product = udev_device_get_sysattr_value(usb_device, "product");
    const char* dev_manufacturer = udev_device_get_sysattr_value(usb_device, "manufacturer");

    if (busnum != nullptr)
        try_parse_number(busnum, desc.bus_number);
    if (devnum != nullptr)
        try_parse_number(devnum, desc.device_number);
    if (dev_id_product != nullptr)
        desc.id_product = dev_id_product;
    if (dev_id_vendor != nullptr)
        desc.id_vendor = dev_id_vendor;
    if (dev_product != nullptr)
        desc.product = dev_product;
    if (dev_manufacturer != nullptr)
        desc.manufacturer = dev_manufacturer;
    if (syspath != nullptr)
        desc.path = syspath;
    if (usb_syspath != nullptr)
        desc.hw_path = usb_syspath;

    desc.topology_depth = get_topology_depth(usb_device);
}

bool try_find_device(udev* udev, udev_enumerate* enumerate, udev_device*& device, udev_device*& usb_device)
{
    udev_enumerate_scan_devices(enumerate);

    udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    if (devices == nullptr)
    {
        return false;
    }

    device = udev_device_new_from_syspath(udev, udev_list_entry_get_name(devices));
    if (device == nullptr)
    {
        return false;
    }

    usb_device = udev_device_get_parent_with_subsystem_devtype(device, "usb", "usb_device");

    if (usb_device == nullptr)
    {
        return false;
    }

    return true;
}

bool try_find_device(udev_device* device, udev_device*& usb_device)
{
    usb_device = udev_device_get_parent_with_subsystem_devtype(device, "usb", "usb_device");

    if (usb_device == nullptr)
    {
        return false;
    }

    return true;
}

int get_topology_depth(udev_device* device)
{
    int depth = 0;
    udev_device* parent = device;
    while (true)
    {
         parent = udev_device_get_parent(parent);
         const char* subsystem = udev_device_get_subsystem(parent);
         if (subsystem == nullptr || strlen(subsystem) == 0)
            break;
         depth++;
    }
    return depth;
}

std::vector<device_description> get_sibling_audio_devices(const device_description& desc)
{
    return get_sibling_devices([](udev_enumerate* enumerate) {
        udev_enumerate_add_match_subsystem(enumerate, "sound");
        udev_enumerate_add_match_sysname(enumerate, "card*");
        udev_enumerate_add_match_property(enumerate, "ID_BUS", "usb");
    }, desc);
}

std::vector<device_description> get_sibling_serial_ports(const device_description& desc)
{
    return get_sibling_devices([](udev_enumerate* enumerate) {
        udev_enumerate_add_match_subsystem(enumerate, "tty");
        udev_enumerate_add_match_property(enumerate, "ID_BUS", "usb");
    }, desc);
}

std::vector<device_description> get_sibling_devices(std::function<void(udev_enumerate*)> filter, const device_description& desc)
{
    std::vector<device_description> siblings;

    udev* udev = udev_new();
    if (udev == nullptr)
        return siblings;

    udev_device* dev = nullptr;

    // Get device from path
    dev = udev_device_new_from_syspath(udev, desc.path.c_str());
    if (dev == nullptr)
    {
        udev_unref(udev);
        return siblings;        
    }

    // Get the corresponding USB device
    udev_device* usb_dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
    if (usb_dev == nullptr)
    {
        udev_device_unref(dev);
        udev_unref(udev);
        return siblings;        
    }

    // Get the USB device's parent USB device
    udev_device* parent_usb_dev = udev_device_get_parent_with_subsystem_devtype(usb_dev, "usb", "usb_device");
    if (parent_usb_dev == nullptr)
    {
        udev_device_unref(dev);
        udev_unref(udev);
        return siblings;        
    }

    udev_enumerate* enumerate = udev_enumerate_new(udev);
    if (enumerate == nullptr)
    {
        udev_device_unref(dev);
        udev_unref(udev);
        return siblings;
    }

    udev_enumerate_add_match_parent(enumerate, parent_usb_dev);

    filter(enumerate);

    udev_enumerate_scan_devices(enumerate);
 
    udev_list_entry *sibling_list_entry = nullptr;
    udev_device *sibling_dev = nullptr;
    udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(sibling_list_entry, devices)
    {
        const char *path = udev_list_entry_get_name(sibling_list_entry);
        sibling_dev = udev_device_new_from_syspath(udev, path);      
        udev_device* sibling_usb_device;
        device_description sibling_desc;
        if (try_get_device_description(sibling_dev, sibling_usb_device, sibling_desc))
            siblings.push_back(sibling_desc);
        udev_device_unref(sibling_dev);
    }

    udev_device_unref(dev);
    udev_enumerate_unref(enumerate);
    udev_unref(udev);    

    return siblings;
}

std::vector<audio_device_info> get_audio_devices(const device_description& desc)
{
    std::vector<audio_device_info> devices;

    udev* udev = udev_new();
    if (!udev)
        return devices;

    udev_device* dev = nullptr;

    // Get device from path
    dev = udev_device_new_from_syspath(udev, desc.path.c_str());
    if (dev == nullptr)
    {
        udev_unref(udev);
        return devices;
    }

    const char* card_id_str = udev_device_get_sysattr_value(dev, "number");
    int card_id;
    if (card_id_str != nullptr && try_parse_number(card_id_str, card_id))
        devices = get_audio_devices(card_id);

    udev_device_unref(dev);
    udev_unref(udev);

    return devices;
}

bool try_get_serial_port(const device_description& desc, serial_port& port)
{
    udev* udev = udev_new();
    if (!udev)
        return false;

    udev_device* dev = nullptr;

    // Get device from path
    dev = udev_device_new_from_syspath(udev, desc.path.c_str());
    if (dev == nullptr)
    {
        udev_unref(udev);
        return false;
    }

    bool found = false;

    const char* devnode = udev_device_get_devnode(dev);

    for (const serial_port& p : get_serial_ports())
    {
        if (p.name == devnode)
        {
            port = p;
            found = true;
            break;
        }
    }

    udev_device_unref(dev);
    udev_unref(udev);

    return found;
}

std::string to_json(const device_description& d, bool wrapping_object, int tabs)
{
    std::string s;
    if (wrapping_object)
    {
        s.append("{\n");
    }
    s.append("    \"bus_number\": \"" + std::to_string(d.bus_number) + "\",\n");
    s.append("    \"device_number\": \"" + std::to_string(d.device_number) + "\",\n");
    s.append("    \"id_product\": \"" + d.id_product + "\",\n");
    s.append("    \"id_vendor\": \"" + d.id_vendor + "\",\n");
    s.append("    \"device_manufacturer\": \"" + d.manufacturer + "\",\n");
    s.append("    \"path\": \"" + d.path + "\",\n");
    s.append("    \"hw_path\": \"" + d.hw_path + "\",\n");
    s.append("    \"product\": \"" + d.product + "\",\n");
    s.append("    \"topology_depth\": \"" + std::to_string(d.topology_depth) + "\"");
    if (wrapping_object)
    {
        s.append("\n");
        s.append("}");
    }
    insert_tabs(s, tabs);
    return s;
}
