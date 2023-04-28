// find_devices.cpp
// Alsa audio device finding utility.
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

#include "find_devices.h"

#include <locale>

#include <alsa/asoundlib.h>

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

std::string audio_device::hw_id() const
{
    return std::string("hw:") + std::to_string(card_id) + "," + std::to_string(device_id);
}

std::string audio_device::plughw_id() const
{
    return std::string("plughw:") + std::to_string(card_id) + "," + std::to_string(device_id);
}

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
        return std::string("capture|playback");
    return std::string("");
}

std::string audio_device::to_string() const
{
    return std::string("card id: '") + std::to_string(card_id) + "', " +
        "device id: '" + std::to_string(device_id) +
        "', name: '" + name +
        "', desc: '" + description +
        "', type: '" + ::to_string(type) + "'";
}

std::string to_json_string(const std::vector<audio_device>& devices)
{
    std::string s;
    s.append("{\n");
    s.append("    \"devices\": [\n");
    for (int i = 0; i < devices.size(); i++)
    {
        const audio_device& d = devices[i];
        s.append("        {\n");
        s.append("            \"card_id\": " + std::to_string(d.card_id) + ",\n");
        s.append("            \"device_id\": " + std::to_string(d.device_id) + ",\n");
        s.append("            \"plughw_id\": \"" + d.plughw_id() + "\",\n");
        s.append("            \"hw_id\": \"" + d.hw_id() + "\",\n");
        s.append("            \"name\": \"" + d.name + "\",\n");
        s.append("            \"description\": \"" + d.description + "\",\n");
        s.append("            \"type\": \"" + to_string(d.type) + "\"\n");
        s.append("        }");
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

std::string to_json_string(const audio_device& d)
{
    std::string s;
    s.append("{\n");
    s.append("    \"card_id\": " + std::to_string(d.card_id) + ",\n");
    s.append("    \"device_id\": " + std::to_string(d.device_id) + ",\n");
    s.append("    \"plughw_id\": \"" + d.plughw_id() + "\",\n");
    s.append("    \"hw_id\": \"" + d.hw_id() + "\",\n");
    s.append("    \"name\": \"" + d.name + "\",\n");
    s.append("    \"description\": \"" + d.description + "\",\n");
    s.append("    \"type\": \"" + to_string(d.type) + "\"\n");
    s.append("}\n");
    return s;
}

std::vector<audio_device> get_audio_devices(const audio_device_type& type)
{
    snd_ctl_t* handle;
    snd_ctl_card_info_t* info;
    snd_pcm_info_t* pcminfo;
    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo);
    int card = -1;
    int status = -1;
    snd_pcm_stream_t stream = type == audio_device_type::capture ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK;
    std::vector<struct audio_device> devices;

    while (true)
    {
        status = snd_card_next(&card);
        if (card < 0)
            break;

        char name[32];
        sprintf(name, "hw:%d", card);

        status = snd_ctl_open(&handle, name, 0);

        //status = snd_ctl_card_info(handle, info);

        int device = -1;

        while (true)
        {
            status = snd_ctl_pcm_next_device(handle, &device);
            if (status < 0 || device < 0)
                break;

            snd_pcm_info_set_device(pcminfo, device);
            snd_pcm_info_set_subdevice(pcminfo, 0);
            snd_pcm_info_set_stream(pcminfo, stream);

            status = snd_ctl_pcm_info(handle, pcminfo);
            if (status < 0)
                continue;

            struct audio_device d;
            d.type = type;
            d.card_id = card;
            d.device_id = device;

            char* name = nullptr;
            status = snd_card_get_name(card, &name);
            d.name = name;

            char* longname = nullptr;
            status = snd_card_get_longname(card, &longname);
            d.description = longname;

            devices.emplace_back(std::move(d));
        }

        snd_ctl_close(handle);
    }

    return devices;
}

std::vector<audio_device> get_audio_devices()
{
    std::vector<audio_device> devices = get_audio_devices(audio_device_type::capture);
    std::vector<audio_device> playbackDevices = get_audio_devices(audio_device_type::playback);
    size_t deviceCount = devices.size();
    for (size_t i = 0; i < deviceCount; i++)
    {
        for (size_t j = 0; j < playbackDevices.size(); j++)
        {
            if (devices[i].card_id == playbackDevices[j].card_id &&
                devices[i].device_id == playbackDevices[j].device_id)
            {
                devices[i].type = devices[i].type | playbackDevices[j].type;
            }
            else
            {
                devices.push_back(playbackDevices[j]);
            }
        }
    }
    return devices;
}

bool match_device(const audio_device& d, const audio_device_match& m)
{
    std::string dNameLower = to_lower(d.name);
    std::string nameLower = to_lower(m.device_name_filter);
    std::string dDescLower = to_lower(d.description);
    std::string descLower = to_lower(m.device_desc_filter);

    if (nameLower.size() > 0 && descLower.size() > 0)
    {
        if (dNameLower.find(nameLower) == std::string::npos || dDescLower.find(descLower) == std::string::npos)
        {
            return false;
        }
    }
    else
    {
        if ((nameLower.size() > 0 && dNameLower.find(nameLower) == std::string::npos) ||
            (descLower.size() > 0 && dDescLower.find(descLower) == std::string::npos))
        {
            return false;
        }
    }

    if (m.playback_and_capture && !m.playback_or_capture &&
        !(enum_device_type_has_flag(d.type, audio_device_type::playback) && enum_device_type_has_flag(d.type, audio_device_type::capture)))
    {
        return false;
    }
    if (!m.playback_and_capture && m.playback_or_capture &&
        !(enum_device_type_has_flag(d.type, audio_device_type::playback) || enum_device_type_has_flag(d.type, audio_device_type::capture)))
    {
        return false;
    }
    if (!m.playback_and_capture && !m.playback_or_capture && (m.playback || m.capture))
    {
        if (m.playback && !enum_device_type_has_flag(d.type, audio_device_type::playback))
        {
            return false;
        }
        if (m.capture && !enum_device_type_has_flag(d.type, audio_device_type::capture))
        {
            return false;
        }
    }

    return true;
}

std::vector<audio_device> get_audio_devices(const audio_device_match& m)
{
    std::vector<audio_device> matched_devices;
    std::vector<audio_device> devices = get_audio_devices();
    for (const auto& d : devices)
    {
        if (match_device(d, m))
        {
            matched_devices.emplace_back(std::move(d));
        }
    }
    return matched_devices;
}

bool can_use_device(const audio_device& device)
{
    return true;
}
