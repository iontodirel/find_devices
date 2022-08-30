// find_devices.h
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

#pragma once

#include <vector>
#include <string>
#include <optional>

enum class audio_device_type : int
{
    uknown = 0,
    playback = 1,
    capture = 2
};

struct audio_device
{
    int card_id = -1;
    int device_id = -1;
    std::string name;
    std::string description;
    audio_device_type type = audio_device_type::uknown;

    std::string hw_id() const;
    std::string plughw_id() const;
    std::string to_string() const;
};

bool can_use_device(const audio_device&);

struct audio_device_match;

std::vector<audio_device> get_audio_devices(const audio_device_type& type);
std::vector<audio_device> get_audio_devices();
std::vector<audio_device> get_audio_devices(const audio_device_match& m);
bool match_device(const audio_device& d, const audio_device_match& m);
std::string to_json_string(const audio_device& d);
std::string to_json_string(const std::vector<audio_device>& devices);

struct audio_device_match
{
    std::string device_name_filter = "";
    std::string device_desc_filter = "";
    bool playback = false;
    bool capture = false;
    bool playback_or_capture = false;
    bool playback_and_capture = false;
};

audio_device_type operator|(const audio_device_type& l, const audio_device_type& r);
audio_device_type operator&(const audio_device_type& l, const audio_device_type& r);

bool enum_device_type_has_flag(const audio_device_type& deviceType, const audio_device_type& flag);

std::string to_string(const audio_device_type& deviceType);
