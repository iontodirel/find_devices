﻿// find_devices.hpp
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
#include <locale>
#include <memory>
#include <sstream>

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

std::string to_string(const audio_device_info&);
std::string to_json(const audio_device_info& d, bool wrapping_object = true, int tabs = 0);
std::string to_json(const std::vector<audio_device_info>& devices);

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

bool can_use_serial_port(const serial_port&);

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