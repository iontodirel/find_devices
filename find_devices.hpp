// **************************************************************** //
// find_devices - Audio device and serial ports search utility      // 
// Version 0.1.0                                                    //
// https://github.com/iontodirel/find_devices                       //
// Copyright (c) 2023 Ion Todirel                                   //
// **************************************************************** //
//
// find_devices.hpp
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

#pragma once

#include <vector>
#include <string>
#include <locale>
#include <sstream>
#include <optional>

// **************************************************************** //
//                                                                  //
// UTILITIES                                                        //
//                                                                  //
// **************************************************************** //

namespace
{
    bool try_parse_number(std::string str, int& number)
    {
        if (str.size() == 0)
            return false;
        int maybe_number = -1;
        std::istringstream iss(str);
        iss >> std::noskipws >> maybe_number;
        bool result = !iss.fail() && iss.eof();
        if (result)
            number = maybe_number;
        return result;
    }

    bool try_parse_number(std::string str, std::optional<int>& number)
    {
        int maybe_number = -1;
        if (try_parse_number(str, maybe_number))
        {
            number = maybe_number;
            return true;
        }
        return false;
    }
}

// **************************************************************** //
//                                                                  //
// AUDIO DEVICES                                                    //
//                                                                  //
// **************************************************************** //

enum class audio_device_type : int
{
    uknown = 0,
    playback = 1,
    capture = 2
};

audio_device_type operator|(const audio_device_type& l, const audio_device_type& r);
audio_device_type operator&(const audio_device_type& l, const audio_device_type& r);

bool enum_device_type_has_flag(const audio_device_type& deviceType, const audio_device_type& flag);

bool try_parse_audio_device_type(const std::string& type_str, audio_device_type& type);

std::string to_string(const audio_device_type& deviceType);

struct audio_device_info
{
    std::string hw_id;
    std::string plughw_id;
    int card_id = -1;
    int device_id = -1;
    std::string name;
    std::string stream_name;
    std::string description;
    audio_device_type type = audio_device_type::uknown;
};

std::vector<audio_device_info> get_audio_devices();
bool can_use_audio_device(const audio_device_info& device);
bool test_audio_device(const audio_device_info& device);

std::string to_string(const audio_device_info&);
std::string to_json(const audio_device_info& d, bool wrapping_object = true, int tabs = 0);
std::string to_json(const std::vector<audio_device_info>& devices);

// **************************************************************** //
//                                                                  //
// AUDIO DEVICE VOLUME                                              //
//                                                                  //
// **************************************************************** //

enum class audio_device_channel_id
{
    front_left,
    front_right,
    front_center,
    rear_left,
    rear_right,
    rear_center,
    woofer,
    side_left,
    side_right,
    mono,
    none
};

bool try_parse_audio_device_channel_id(const std::string& channel_str, audio_device_channel_id& type);
bool try_parse_audio_device_channel_display_name(const std::string& channel_str, audio_device_channel_id& type);

struct audio_device_channel
{
    std::string name;
    int volume = 0;
    int volume_linearized = 0;
    audio_device_type type = audio_device_type::uknown;
    audio_device_channel_id id = audio_device_channel_id::none;
};

struct audio_device_volume_control
{
    std::string name;
    std::vector<audio_device_channel> channels;
};

struct audio_device_volume_info
{
    audio_device_info audio_device;
    std::vector<audio_device_volume_control> controls;
};

bool try_get_audio_device_volume(const audio_device_info& device, audio_device_volume_info& volume);
bool try_set_audio_device_volume(const audio_device_info& device, const std::string& control_name, const audio_device_channel& channel);
bool try_set_audio_device_volume(const audio_device_info& device, const audio_device_volume_control& control, const audio_device_channel& channel);
bool try_set_audio_device_volume(const audio_device_info& device, const std::string& control_name, const audio_device_channel_id& channel, const audio_device_type& channel_type, int value);

std::string to_json(const audio_device_volume_info& d, bool wrapping_object = true, int tabs = 0);

std::string to_string(const audio_device_channel_id& type);

// **************************************************************** //
//                                                                  //
// SERIAL PORTS                                                     //
//                                                                  //
// **************************************************************** //

struct serial_port
{
    std::string name;
    std::string description;
    std::string manufacturer;
    std::string device_serial_number;
};

bool can_use_serial_port(const serial_port& p);
bool test_serial_port(const serial_port& device);

std::vector<serial_port> get_serial_ports();

std::string to_json(const serial_port& p, bool wrapping_object = true, int tabs = 0);

// **************************************************************** //
//                                                                  //
// DEVICE INFO                                                      //
//                                                                  //
// **************************************************************** //

struct device_description
{
    int bus_number = -1;
    int device_number = -1;
    std::string path;
    std::string hw_path;
    std::string id_vendor;
    std::string id_product;
    std::string product;
    std::string manufacturer;
    int topology_depth = -1;
};

bool try_get_device_description(const audio_device_info& d, device_description& device);
bool try_get_device_description(const serial_port& d, device_description& device);

std::vector<device_description> get_sibling_audio_devices(const device_description& desc);

std::vector<device_description> get_sibling_serial_ports(const device_description& desc);

std::vector<audio_device_info> get_audio_devices(const device_description& desc);

bool try_get_serial_port(const device_description& desc, serial_port& p);

std::string to_json(const device_description& d, bool wrapping_object = true, int tabs = 0);
