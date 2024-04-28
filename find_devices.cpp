// **************************************************************** //
// find_devices - Audio device and serial ports search utility      // 
// Version 0.1.0                                                    //
// https://github.com/iontodirel/find_devices                       //
// Copyright (c) 2023 Ion Todirel                                   //
// **************************************************************** //
//
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
//
// Part of the functions get_channel_volume_linearized, set_channel_volume_linearized and use_linear_dB_scale
// copied from code by Clemens Ladisch <clemens@ladisch.de> under ISC license.
// 
// Copyright (c) 2010 Clemens Ladisch <clemens@ladisch.de>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

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

namespace 
{
    std::string to_lower(const std::string& str)
    {
        std::locale loc;
        std::string s;
        s.resize(str.size());
        for (size_t i = 0; i < str.size(); i++)
            s[i] = std::tolower(str[i], loc);
        return s;
    }
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
bool test_audio_device(const audio_device_info& device);
audio_device_channel_id parse_audio_device_channel_id(int channel_id);
int parse_audio_device_channel_type(audio_device_channel_id type);
int get_playback_channel_volume(snd_mixer_elem_t* elem, int channel_id);
bool try_get_playback_channel_volume(snd_mixer_elem_t* elem, int channel_id, int& result);
bool try_get_capture_channel_volume(snd_mixer_elem_t* elem, int channel_id, int& result);
bool try_set_playback_channel_volume(snd_mixer_elem_t* elem, int channel_id, int value);
bool try_set_capture_channel_volume(snd_mixer_elem_t* elem, int channel_id, int value);
bool use_linear_dB_scale(long dBmin, long dBmax);
bool try_get_channel_volume_linearized(snd_mixer_elem_t* elem, int channel_id, int& result, std::function<int(snd_mixer_elem_t *elem, long *min, long *max)> get_db_range, std::function<int(snd_mixer_elem_t *elem, long *min, long *max)> get_volume_range, std::function<int(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value)> get_volume, std::function<int(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value)> get_db);
bool try_get_playback_channel_volume_linearized(snd_mixer_elem_t* elem, int channel_id, int& result);
bool try_get_capture_channel_volume_linearized(snd_mixer_elem_t* elem, int channel_id, int& result);
bool try_get_channel_volume(snd_mixer_elem_t* elem, int channel_id, int& result, std::function<int(snd_mixer_elem_t *elem, long *min, long *max)> get_volume_range, std::function<int(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value)> get_volume);

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

bool try_parse_audio_device_type(const std::string& type_str, audio_device_type& type)
{
    bool result = true;
    if (type_str == "playback")
    {
        type = audio_device_type::playback;
    }
    else if (type_str == "capture")
    {
        type = audio_device_type::capture;
    }
    else if (type_str == "uknown")
    {
        type = audio_device_type::uknown;
    }
    else
    {
        type = audio_device_type::uknown;
        result = false;
    }
    return result;
}

bool try_parse_audio_device_channel_id(const std::string& channel_str, audio_device_channel_id& type)
{
    std::string lowercase_channel_str = to_lower(channel_str);

    if (lowercase_channel_str == "front_left")
    {
        type = audio_device_channel_id::front_left;
        return true;
    }
    else if (lowercase_channel_str == "front_right")
    {
        type = audio_device_channel_id::front_right;
        return true;
    }
    else if (lowercase_channel_str == "front_center")
    {
        type = audio_device_channel_id::front_center;
        return true;
    } 
    else if (lowercase_channel_str == "rear_left")
    {
        type = audio_device_channel_id::rear_left;
        return true;
    }
    else if (lowercase_channel_str == "rear_right") 
    {
        type = audio_device_channel_id::rear_right;
        return true;
    }
    else if (lowercase_channel_str == "rear_center") 
    {
        type = audio_device_channel_id::rear_center;
        return true;
    }
    else if (lowercase_channel_str == "woofer") 
    {
        type = audio_device_channel_id::woofer;
        return true;
    }
    else if (lowercase_channel_str == "side_left") 
    {
        type = audio_device_channel_id::side_left;
        return true;
    }
    else if (lowercase_channel_str == "side_right") 
    {
        type = audio_device_channel_id::side_right;
        return true;
    }
    else if (lowercase_channel_str == "mono") 
    {
        type = audio_device_channel_id::mono;
        return true;
    }
    else if (lowercase_channel_str == "none")
    {
        type = audio_device_channel_id::none;
        return true;
    }

    return false;
}

bool try_parse_audio_device_channel_display_name(const std::string& channel_str, audio_device_channel_id& type)
{
    std::string lowercase_channel_str = to_lower(channel_str);

    if (lowercase_channel_str == "front left")
    {
        type = audio_device_channel_id::front_left;
        return true;
    }
    else if (lowercase_channel_str == "front right")
    {
        type = audio_device_channel_id::front_right;
        return true;
    }
    else if (lowercase_channel_str == "front center")
    {
        type = audio_device_channel_id::front_center;
        return true;
    } 
    else if (lowercase_channel_str == "rear left")
    {
        type = audio_device_channel_id::rear_left;
        return true;
    }
    else if (lowercase_channel_str == "rear right") 
    {
        type = audio_device_channel_id::rear_right;
        return true;
    }
    else if (lowercase_channel_str == "rear center") 
    {
        type = audio_device_channel_id::rear_center;
        return true;
    }
    else if (lowercase_channel_str == "woofer") 
    {
        type = audio_device_channel_id::woofer;
        return true;
    }
    else if (lowercase_channel_str == "side left") 
    {
        type = audio_device_channel_id::side_left;
        return true;
    }
    else if (lowercase_channel_str == "side right") 
    {
        type = audio_device_channel_id::side_right;
        return true;
    }
    else if (lowercase_channel_str == "mono") 
    {
        type = audio_device_channel_id::mono;
        return true;
    }
    else if (lowercase_channel_str == "none")
    {
        type = audio_device_channel_id::none;
        return true;
    }

    return false;
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

std::string to_string(const audio_device_channel_id& type)
{
    switch (type)
    {
    case audio_device_channel_id::front_left:
        return "front_left";
    case audio_device_channel_id::front_right:
        return "front_right";
    case audio_device_channel_id::front_center:
        return "front_center";
    case audio_device_channel_id::rear_left:
        return "rear_left";
    case audio_device_channel_id::rear_right:
        return "rear_right";
    case audio_device_channel_id::rear_center:
        return "rear_center";
    case audio_device_channel_id::woofer:
        return "woofer";
    case audio_device_channel_id::side_left:
        return "side_left";
    case audio_device_channel_id::side_right:
        return "side_right";
    case audio_device_channel_id::mono:
        return "mono";
    case audio_device_channel_id::none:
        return "none";
    default:
        return "";
    }
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
    snd_pcm_t* handle;

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
// AUDIO DEVICE VOLUME                                              //
//                                                                  //
//                                                                  //
// **************************************************************** //

bool try_get_audio_device_volume(const audio_device_info& device, audio_device_volume_info& volume)
{
    bool result = true;

    volume.audio_device = device;
    volume.controls.clear();

    int err;
    snd_mixer_t* handle;
    snd_mixer_selem_id_t* sid;

    snd_mixer_open(&handle, 0);

    // NOTE: can get the error message with snd_strerror(err)

    if ((err = snd_mixer_open(&handle, 0)) < 0)
    {
        return false;
    }

    if ((err = snd_mixer_attach(handle, ("hw:" + std::to_string(device.card_id)).c_str())) < 0)
    {
        snd_mixer_close(handle);
        return false;
    }

    if ((err = snd_mixer_selem_register(handle, nullptr, nullptr)) < 0)
    {
        snd_mixer_close(handle);
        return false;
    }

    if ((err = snd_mixer_load(handle)) < 0)
    {
        snd_mixer_close(handle);
        return false;
    }

    snd_mixer_selem_id_malloc(&sid);
    snd_mixer_selem_id_set_index(sid, 0);

    snd_mixer_elem_t* elem = nullptr;

    for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem))
    {
        snd_mixer_selem_get_id(elem, sid);

        if (!snd_mixer_selem_has_capture_volume(elem) && !snd_mixer_selem_has_playback_volume(elem))
        {
            continue;
        }

        audio_device_volume_control volume_control;

        volume_control.name = snd_mixer_selem_get_name(elem);

        for (int channel_id = 0; channel_id <= SND_MIXER_SCHN_REAR_CENTER; channel_id++)
        {
            if (!snd_mixer_selem_has_playback_channel(elem, (snd_mixer_selem_channel_id_t)channel_id) &&
                !snd_mixer_selem_has_capture_channel(elem, (snd_mixer_selem_channel_id_t)channel_id))
                continue;

            audio_device_channel channel;

            channel.name = snd_mixer_selem_channel_name((snd_mixer_selem_channel_id_t)channel_id);
            channel.id = parse_audio_device_channel_id(channel_id);

            if (snd_mixer_selem_has_playback_volume(elem) && snd_mixer_selem_has_playback_channel(elem, (snd_mixer_selem_channel_id_t)channel_id))
            {
                channel.type = audio_device_type::playback;
                result = try_get_playback_channel_volume(elem, channel_id, channel.volume);
                if (!result)
                    break;
                result = try_get_playback_channel_volume_linearized(elem, channel_id, channel.volume_linearized);
                if (!result)
                    break;
                volume_control.channels.push_back(channel);
            }
            if (snd_mixer_selem_has_capture_volume(elem) && snd_mixer_selem_has_capture_channel(elem, (snd_mixer_selem_channel_id_t)channel_id))
            {
                channel.type = audio_device_type::capture;
                result = try_get_capture_channel_volume(elem, channel_id, channel.volume);
                if (!result)
                    break;
                result = try_get_capture_channel_volume_linearized(elem, channel_id, channel.volume_linearized);
                if (!result)
                    break;
                volume_control.channels.push_back(channel);
            }

            // if (channel_id == 0 && snd_mixer_selem_is_playback_mono(elem))
            // {
            //     break;
            // }
        }

        volume.controls.push_back(volume_control);
    }

    snd_mixer_selem_id_free(sid);
    snd_mixer_close(handle);

    return result;
}

bool try_set_audio_device_volume(const audio_device_info& device, const std::string& control_name, const audio_device_channel& channel)
{
    return try_set_audio_device_volume(device, control_name, channel.id, channel.type, channel.volume);
}

bool try_set_audio_device_volume(const audio_device_info& device, const audio_device_volume_control& control, const audio_device_channel& channel)
{
    return try_set_audio_device_volume(device, control.name, channel.id, channel.type, channel.volume);
}

bool try_set_audio_device_volume(const audio_device_info& device, const std::string& control_name, const audio_device_channel_id& channel, const audio_device_type& channel_type, int volume)
{
    bool result = true;

    int err;
    snd_mixer_t* handle;
    snd_mixer_selem_id_t* sid;

    // NOTE: can get the error message with snd_strerror(err)

    if ((err = snd_mixer_open(&handle, 0)) < 0)
    {
        return false;
    }

    if ((err = snd_mixer_attach(handle, ("hw:" + std::to_string(device.card_id)).c_str())) < 0)
    {
        snd_mixer_close(handle);
        return false;
    }

    if ((err = snd_mixer_selem_register(handle, nullptr, nullptr)) < 0)
    {
        snd_mixer_close(handle);
        return false;
    }

    if ((err = snd_mixer_load(handle)) < 0)
    {
        snd_mixer_close(handle);
        return false;
    }

    snd_mixer_selem_id_malloc(&sid);
    snd_mixer_selem_id_set_index(sid, 0);

    snd_mixer_elem_t* elem = nullptr;

    for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem))
    {
        snd_mixer_selem_get_id(elem, sid);

        std::string element_name = snd_mixer_selem_get_name(elem);
        int channel_id = parse_audio_device_channel_type(channel);

        if (element_name != control_name)
        {
            continue;
        }

        if (channel_type == audio_device_type::playback && snd_mixer_selem_has_playback_volume(elem) && snd_mixer_selem_has_playback_channel(elem, (snd_mixer_selem_channel_id_t)channel_id))
        {
            result = try_set_playback_channel_volume(elem, channel_id, volume);
            break;
        }
        else if (channel_type == audio_device_type::capture && snd_mixer_selem_has_capture_volume(elem) && snd_mixer_selem_has_capture_channel(elem, (snd_mixer_selem_channel_id_t)channel_id))
        {
            result = try_set_capture_channel_volume(elem, channel_id, volume);
            break;
        }
        else
        {
            result = false;
            break;
        }
    }

    snd_mixer_selem_id_free(sid);
    snd_mixer_close(handle);

    return result;
}

audio_device_channel_id parse_audio_device_channel_id(int channel_id)
{
    switch (channel_id)
    {
    case 0:
        // if (snd_mixer_selem_has_playback_channel(elem, SND_MIXER_SCHN_MONO))
        // {
        //     channel.channel = audio_device_channel_type::mono;
        // }
        // else
        // {
        return audio_device_channel_id::front_left;
        //}
    case 1:
        return audio_device_channel_id::front_right;
    case 2:
        return audio_device_channel_id::rear_left;
    case 3:
        return audio_device_channel_id::rear_right;
    case 4:
        return audio_device_channel_id::front_center;
    case 5:
        return audio_device_channel_id::woofer;
    case 6:
        return audio_device_channel_id::side_left;
    case 7:
        return audio_device_channel_id::side_right;
    case 8:
        return audio_device_channel_id::rear_center;
    default:
        return audio_device_channel_id::none;
    }
}

int parse_audio_device_channel_type(audio_device_channel_id type)
{
    switch (type)
    {
    case audio_device_channel_id::front_left:
        return 0;
    case audio_device_channel_id::front_right:
        return 1;
    case audio_device_channel_id::rear_left:
        return 2;
    case audio_device_channel_id::rear_right:
        return 3;
    case audio_device_channel_id::front_center:
        return 4;
    case audio_device_channel_id::woofer:
        return 5;
    case audio_device_channel_id::side_left:
        return 6;
    case audio_device_channel_id::side_right:
        return 7;
    case audio_device_channel_id::rear_center:
        return 8;
    default:
        return 0;
    }
}

bool use_linear_dB_scale(long db_min, long db_max)
{
    constexpr int MAX_LINEAR_DB_SCALE =	24;
	return (db_max - db_min) <= (MAX_LINEAR_DB_SCALE * 100);
}

bool try_get_channel_volume_linearized(snd_mixer_elem_t* elem, int channel_id, int& result, std::function<int(snd_mixer_elem_t *elem, long *min, long *max)> get_db_range, std::function<int(snd_mixer_elem_t *elem, long *min, long *max)> get_volume_range, std::function<int(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value)> get_volume, std::function<int(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value)> get_db)
{
    
    // Some code from this function was copied from https://github.com/alsa-project/alsa-utils/blob/master/alsamixer/volume_mapping.c
    // Copyright (c) 2010 Clemens Ladisch <clemens@ladisch.de>
    // Licensed under ISC license.
    // See copyright message at the top of this file.

    long min, max, value;
    double normalized, min_norm;
    int err;    

    err = get_db_range(elem, &min, &max);
    if (err < 0 || min >= max)
    {
        err = get_volume_range(elem, &min, &max);
        if (err < 0 || min == max)
        {
			return false;
        }
        err = get_volume(elem, (snd_mixer_selem_channel_id_t)channel_id, &value);
        if (err < 0)
        {
			return false;
        }
        double result_double = ((value - min) * 100.0) / (double)(max - min);
        result = (int)std::rint(result_double);
        return true;
    }

    err = get_db(elem, (snd_mixer_selem_channel_id_t)channel_id, &value);
    if (err < 0)
    {
		return false;
    }

    if (use_linear_dB_scale(min, max))
    {
		double result_double = ((value - min) * 100.0) / (double)(max - min);
        result = (int)std::rint(result_double);
        return true;
    }
    
   	normalized = pow(10, (value - max) / 6000.0);
    if (min != SND_CTL_TLV_DB_GAIN_MUTE)
    {
		min_norm = pow(10, (min - max) / 6000.0);
		normalized = (normalized - min_norm) / (1 - min_norm);
	}
    
    result = (int)std::rint(normalized * 100.0);

    return true;
}

bool try_get_playback_channel_volume_linearized(snd_mixer_elem_t* elem, int channel_id, int& result)
{
    return try_get_channel_volume_linearized(elem, channel_id, result,
        [](snd_mixer_elem_t *elem, long *min, long *max) { return snd_mixer_selem_get_playback_dB_range(elem, min, max); },
        [](snd_mixer_elem_t *elem, long *min, long *max) { return snd_mixer_selem_get_playback_volume_range(elem, min, max); },
        [](snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value) { return snd_mixer_selem_get_playback_volume(elem, channel, value); },
        [](snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value) { return snd_mixer_selem_get_playback_dB(elem, channel, value); });
}

bool try_get_capture_channel_volume_linearized(snd_mixer_elem_t* elem, int channel_id, int& result)
{
    return try_get_channel_volume_linearized(elem, channel_id, result,
        [](snd_mixer_elem_t *elem, long *min, long *max) { return snd_mixer_selem_get_capture_dB_range(elem, min, max); },
        [](snd_mixer_elem_t *elem, long *min, long *max) { return snd_mixer_selem_get_capture_volume_range(elem, min, max); },
        [](snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value) { return snd_mixer_selem_get_capture_volume(elem, channel, value); },
        [](snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value) { return snd_mixer_selem_get_capture_dB(elem, channel, value); });
}

bool try_get_channel_volume(snd_mixer_elem_t* elem, int channel_id, int& result, std::function<int(snd_mixer_elem_t *elem, long *min, long *max)> get_volume_range, std::function<int(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value)> get_volume)
{
    long min = 0, max = 100, value = 0;
    int err;
    err = get_volume_range(elem, &min, &max);
    if (err < 0)
    {
        return false;
    }
    err = get_volume(elem, (snd_mixer_selem_channel_id_t)channel_id, &value);
    if (err < 0)
    {
        return false;
    }
    double result_double = ((value - min) * 100.0 / (double)(max - min));
    result = (int)std::rint(result_double);
    return true;
}

bool try_set_channel_volume(snd_mixer_elem_t* elem, int channel_id, int value, std::function<int(snd_mixer_elem_t *elem, long *min, long *max)> get_volume_range, std::function<int(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long value)> set_volume)
{
    long min = 0, max = 100;
    int err;
    err = get_volume_range(elem, &min, &max);
    if (err < 0)
    {
        return false;
    }
    double value_adjusted_double = ((value * (double)(max - min)) / 100.0) + min;
    long value_adjusted = (long)std::rint(value_adjusted_double);
    err = set_volume(elem, (snd_mixer_selem_channel_id_t)channel_id, value_adjusted);
    if (err < 0)
    {
        return false;
    }
    return true;
}

bool try_get_playback_channel_volume(snd_mixer_elem_t* elem, int channel_id, int& result)
{
    return try_get_channel_volume(elem, channel_id, result,
        [](snd_mixer_elem_t *elem, long *min, long *max) { return snd_mixer_selem_get_playback_volume_range(elem, min, max); },
        [](snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value) { return snd_mixer_selem_get_playback_volume(elem, channel, value); });
}

bool try_set_playback_channel_volume(snd_mixer_elem_t* elem, int channel_id, int value)
{
    return try_set_channel_volume(elem, channel_id, value,
        [](snd_mixer_elem_t *elem, long *min, long *max) { return snd_mixer_selem_get_playback_volume_range(elem, min, max); },
        [](snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long value) { return snd_mixer_selem_set_playback_volume(elem, channel, value); });
}

bool try_get_capture_channel_volume(snd_mixer_elem_t* elem, int channel_id, int& result)
{
    return try_get_channel_volume(elem, channel_id, result,
        [](snd_mixer_elem_t *elem, long *min, long *max) { return snd_mixer_selem_get_capture_volume_range(elem, min, max); },
        [](snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value) { return snd_mixer_selem_get_capture_volume(elem, channel, value); });
}

bool try_set_capture_channel_volume(snd_mixer_elem_t* elem, int channel_id, int value)
{
    return try_set_channel_volume(elem, channel_id, value,
        [](snd_mixer_elem_t *elem, long *min, long *max) { return snd_mixer_selem_get_capture_volume_range(elem, min, max); },
        [](snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long value) { return snd_mixer_selem_set_capture_volume(elem, channel, value); });
}

std::string to_json(const audio_device_volume_info& d, bool wrapping_object, int tabs);
std::string to_json(const audio_device_volume_info& d, std::function<std::string(const audio_device_volume_info& d)> render_device, std::function<std::string(const audio_device_volume_info& d, const audio_device_volume_control& c)> render_control, std::function<std::string(const audio_device_volume_info& d, const audio_device_volume_control& c, const audio_device_channel& ch)> render_channel, bool wrapping_object, int tabs);

std::string to_json(const audio_device_volume_info& d, bool wrapping_object, int tabs)
{
    return to_json(d,
        std::function<std::string(const audio_device_volume_info&)>{},
        std::function<std::string(const audio_device_volume_info&, const audio_device_volume_control&)>{},
        std::function<std::string(const audio_device_volume_info&, const audio_device_volume_control&, const audio_device_channel&)>{},
        wrapping_object,
        tabs);
}

std::string to_json(const audio_device_volume_info& d, std::function<std::string(const audio_device_volume_info& d)> render_device, std::function<std::string(const audio_device_volume_info& d, const audio_device_volume_control& c)> render_control, std::function<std::string(const audio_device_volume_info& d, const audio_device_volume_control& c, const audio_device_channel& ch)> render_channel, bool wrapping_object, int tabs)
{
    std::string s;
    if (wrapping_object)
    {
        s.append("{\n");
    }
    s.append("    \"card_id\": \"" + std::to_string(d.audio_device.card_id) + "\",\n");
    s.append("    \"device_id\": \"" + std::to_string(d.audio_device.device_id) + "\",\n");
    s.append("    \"plughw_id\": \"" + d.audio_device.plughw_id + "\",\n");
    s.append("    \"hw_id\": \"" + d.audio_device.hw_id + "\",\n");
    s.append("    \"name\": \"" + d.audio_device.name + "\",\n");
    s.append("    \"description\": \"" + d.audio_device.description + "\",\n");
    s.append("    \"type\": \"" + to_string(d.audio_device.type) + "\",\n");
    if (render_device)
    {
        std::string render_device_string = render_device(d);
        if (!render_device_string.empty())
        {
            s.append(render_device_string);
        }
    }
    s.append("    \"controls\": [\n");
    for (size_t i = 0; i < d.controls.size(); i++)
    {
        s.append("        {\n");
        s.append("            \"name\": \"" + d.controls[i].name + "\",\n");
        if (render_control)
        {
            std::string render_control_string = render_control(d, d.controls[i]);
            if (!render_control_string.empty())
            {
                s.append(render_control_string);
            }
        }
        s.append("            \"channels\": [\n");
        for (size_t j = 0; j < d.controls[i].channels.size(); j++)
        {
            s.append("                {\n");
            s.append("                    \"name\": \"" + d.controls[i].channels[j].name + "\",\n");
            s.append("                    \"type\": \"" + to_string(d.controls[i].channels[j].type) + "\",\n");
            s.append("                    \"volume\": \"" + std::to_string(d.controls[i].channels[j].volume) + "\",\n");
            s.append("                    \"channel\": \"" + to_string(d.controls[i].channels[j].id) + "\"");
            if (render_channel)
            {
                std::string render_channel_string = render_channel(d, d.controls[i], d.controls[i].channels[j]);
                if (!render_channel_string.empty())
                {
                    s.append(",\n");
                    s.append(render_channel_string);
                }
                else
                {
                    s.append("\n");
                }
            }
            else
            {
                s.append("\n");
            }
            s.append("                }");
            if ((j + 1) < d.controls[i].channels.size())
                s.append(",");
            s.append("\n");
        }
        s.append("            ]\n");
        s.append("        }");
        if ((i + 1) < d.controls.size())
            s.append(",");
        s.append("\n");
    }
    s.append("    ]");
    if (wrapping_object)
    {
        s.append("\n");
        s.append("}");
    }
    insert_tabs(s, tabs);
    return s;
}

bool test_audio_device(const audio_device_info& device)
{
    int err;
    snd_pcm_t* pcm_handle;
    snd_pcm_hw_params_t* hw_params;
    const unsigned int buffer_size = 1024;  // Specify the buffer size in frames
    const unsigned int channels = 2;  // Number of audio channels
    unsigned int sample_rate = 44100;  // Audio sample rate

    // Open the ALSA device for playback
    err = snd_pcm_open(&pcm_handle, device.hw_id.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
    {
        return false;
    }

    // Allocate and initialize hardware parameters object
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(pcm_handle, hw_params);

    // Set hardware parameters for desired configuration
    snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, hw_params, channels);
    snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &sample_rate, 0);

    // Apply the hardware parameters to the PCM device
    err = snd_pcm_hw_params(pcm_handle, hw_params);
    if (err < 0)
    {
        snd_pcm_close(pcm_handle);
        return false;
    }

    // Create a buffer to store the zeros
    short zeros[buffer_size * channels];
    memset(zeros, 0, sizeof(zeros));

    // Write the buffer of zeros to the PCM device
    err = snd_pcm_writei(pcm_handle, zeros, buffer_size);
    if (err < 0)
    {
        snd_pcm_close(pcm_handle);
        return false;
    }

    // Close the PCM device
    snd_pcm_close(pcm_handle);

    return true;
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
        }, desc);
}

std::vector<device_description> get_sibling_serial_ports(const device_description& desc)
{
    return get_sibling_devices([](udev_enumerate* enumerate) {
        udev_enumerate_add_match_subsystem(enumerate, "tty");
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

    udev_list_entry* sibling_list_entry = nullptr;
    udev_device* sibling_dev = nullptr;
    udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(sibling_list_entry, devices)
    {
        const char* path = udev_list_entry_get_name(sibling_list_entry);
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
