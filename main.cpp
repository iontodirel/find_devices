// **************************************************************** //
// find_devices - Audio device and serial ports search utility      // 
// Version 0.1.0                                                    //
// https://github.com/iontodirel/find_devices                       //
// Copyright (c) 2023 Ion Todirel                                   //
// **************************************************************** //
//
// main.cpp
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

#include <map>
#include <fstream>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <iostream>
#include <fstream>
#include <map>
#include <cmath>
#include <algorithm>

#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <fmt/color.h>
#include <cxxopts.hpp>

// **************************************************************** //
//                                                                  //
// UTILITIES                                                        //
//                                                                  //
// **************************************************************** //

#define STR(x) #x
#define STRINGIFY(x) STR(x)

namespace
{
    template <typename... Args>
    void print(bool enable_colors, const fmt::text_style& ts, fmt::format_string<Args...> fmt, Args&&... args)
    {
        fmt::print(enable_colors ? ts : fmt::text_style(), fmt, std::forward<Args>(args)...);
    }

    template<class F, class ...Args>
    void print_stdout(F f, Args... args)
    {
        printf(format(f, args ...).c_str());
    }

    void write_line_to_file(const std::string& path, const std::string& line)
    {
        std::ofstream file;
        file.open(path, std::ios_base::trunc);
        if (file.is_open())
        {
            file << line << "\n";
            file.close();
        }
    }

    bool try_parse_bool(const std::string& s, bool& b)
    {
        if (s == "true")
            b = true;
        else if (s == "false")
            b = false;
        else
            return false;
        return true;
    }

    std::string get_full_path(const std::string& path)
    {
        std::filesystem::path p = path;
        if (p.is_absolute())
            return path;
        return std::filesystem::current_path() / path;
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

    bool try_find_new_filename(const std::filesystem::path& file_name, std::string& new_filename)
    {
        int i = 1;
        int max_count = 1000;
        while (i < max_count)
        {
            new_filename = (file_name.parent_path() / (file_name.stem().string() + std::to_string(i) + file_name.extension().string())).string();

            if (!std::filesystem::exists(new_filename))
                return true;

            i++;
        }
        return false;
    }
}

// **************************************************************** //
//                                                                  //
//                                                                  //
// DATA TYPES                                                       //
//                                                                  //
//                                                                  //
// **************************************************************** //

struct audio_device_filter;
struct serial_port_filter;
enum class search_mode;
enum class included_devices;
struct args;
struct search_result;
struct audio_device_volume_set;
struct audio_device_unique_volume_set;

struct audio_device_filter
{
    std::string name_filter = "";
    std::string desc_filter = "";
    std::string stream_name_filter = "";
    bool playback_only = false; // TODO: use audio_device_type?
    bool capture_only = false; // TODO: use audio_device_type?
    bool playback_or_capture = false; // TODO: use audio_device_type?
    bool playback_and_capture = false; // TODO: use audio_device_type?
    int bus = -1;
    int device = -1;
    int topology = -1;
    std::string path;
};

struct audio_device_volume_set
{
    std::string control_name;
    std::vector<audio_device_channel_id> audio_channels;
    audio_device_type audio_channel_type = audio_device_type::uknown;
    int volume = -1;
    int volume_max_error = 0;
};

struct audio_device_unique_volume_set
{
    audio_device_volume_set volume_set;
    audio_device_volume_info volume;
    audio_device_volume_control control;
    audio_device_channel channel;
    std::string unique_channel_id;
    int property_set_count = 0;
    int volume_control_error = 0;
    int volume_control_error_percent = 0;
};

struct audio_device_volume_probe
{
    int retrieved_volume = 0;
    int set_volume = 0;
    int error = 0;
    int error_percent = 0;
    std::string unique_channel_id;
};

struct serial_port_filter
{
    std::string name_filter = "";
    std::string description_filter = "";
    std::string manufacturer_filter = "";
    std::string device_serial_number = "";
    int bus = -1;
    int device = -1;
    int topology = -1;
    std::string path;
};

enum class search_mode
{
    not_set,
    independent,
    audio_siblings,
    port_siblings
};

enum class included_devices
{
    unknown = 0,
    audio,
    ports,
    all
};

struct args
{
    std::multimap<std::string, std::string> command_line_args;
    audio_device_filter audio_filter;
    serial_port_filter port_filter;
    std::vector<audio_device_volume_set> volume_set;
    bool verbose = true;
    bool help = false;
    bool use_json = false;
    std::string output_file;
    bool disable_write_file = false;
    bool list_properties = false;
    enum search_mode search_mode = search_mode::independent;
    enum included_devices included_devices = included_devices::all;
    std::string config_file = "config.json";
    bool ignore_config = false;
    bool disable_colors = false;
    bool no_stdout = false;
    bool command_line_has_errors = false;
    std::string command_line_error = "";
    bool show_version = false;
    bool disable_volume_control = false;
    bool test_volume_control = false;
    bool probe_volume_control = false;
    std::string direwolf_output_file;
    std::string direwolf_callsign;
    int direwolf_agwport = -1;
    int direwolf_kissport = -1;
};

struct search_result
{
    std::vector<std::pair<audio_device_volume_info, device_description>> devices;
    std::vector<std::pair<serial_port, device_description>> ports;
};

struct option_handler
{
    std::string command_line_arg_name;
    bool has_type = false;
    std::shared_ptr<cxxopts::Value> type;
    std::function<void(const cxxopts::ParseResult&)> read_handler;
};

// **************************************************************** //
//                                                                  //
// PARSE TYPE FUNCTIONS                                             //
//                                                                  //
// **************************************************************** //

void parse_audio_device_type(const std::string& typeStr, args& args)
{
    args.audio_filter.playback_or_capture = false;
    args.audio_filter.playback_only = false;
    args.audio_filter.capture_only = false;
    args.audio_filter.playback_and_capture = false;

    if (typeStr == "playback")
    {
        args.audio_filter.playback_only = true;
    }
    else if (typeStr == "capture")
    {
        args.audio_filter.capture_only = true;
    }
    else if (typeStr == "playback|capture" || typeStr == "playback | capture" ||
        typeStr == "capture|playback" || typeStr == "capture | playback")
    {
        args.audio_filter.playback_or_capture = true;
    }
    else if (typeStr == "playback&capture" || typeStr == "playback & capture" ||
        typeStr == "capture&playback" || typeStr == "capture & playback")
    {
        args.audio_filter.playback_and_capture = true;
    }
}

bool try_parse_channels(const std::string& channels_str, std::vector<audio_device_channel_id>& channels)
{
    std::istringstream ss(channels_str);
    std::string channel_str;

    while (std::getline(ss, channel_str, ','))
    {
        audio_device_channel_id channel_id;
        try_parse_audio_device_channel_display_name(channel_str, channel_id);
        channels.push_back(channel_id);
    }

    return true;
}

bool try_parse_included_devices(const std::string& mode_str, included_devices& mode)
{
    if (mode_str == "audio")
    {
        mode = included_devices::audio;
    }
    else if (mode_str == "ports")
    {
        mode = included_devices::ports;
    }
    else if (mode_str == "all")
    {
        mode = included_devices::all;
    }
    else
    {
        return false;
    }
    return true;
}

bool try_parse_search_mode(const std::string& mode_str, search_mode& mode)
{
    if (mode_str == "independent")
    {
        mode = search_mode::independent;
    }
    else if (mode_str == "audio-siblings")
    {
        mode = search_mode::audio_siblings;
    }
    else if (mode_str == "port-siblings")
    {
        mode = search_mode::port_siblings;
    }
    else
    {
        return false;
    }
    return true;
}

// **************************************************************** //
//                                                                  //
// JSON                                                             //
//                                                                  //
// **************************************************************** //

std::string create_unique_channel_id(const audio_device_info& device, const audio_device_volume_control& control, const audio_device_channel& channel);
std::string to_json(const args& args, const search_result& result, const std::vector<audio_device_unique_volume_set>& audio_set_result, bool volume_control_return_value);

std::string to_json(const audio_device_volume_info& d, const std::vector<audio_device_unique_volume_set>& audio_set_result, bool wrapping_object, int tabs)
{
    return to_json(d,
        std::function<std::string(const audio_device_volume_info&)>{},
        std::function<std::string(const audio_device_volume_info&, const audio_device_volume_control&)>{},
        [&audio_set_result](const audio_device_volume_info& d, const audio_device_volume_control& control, const audio_device_channel& channel)
        {
            std::string result;

            std::string channel_id = create_unique_channel_id(d.audio_device, control, channel);

            auto audio_set = std::find_if(audio_set_result.begin(), audio_set_result.end(), [&channel_id](const audio_device_unique_volume_set& e) {
                return e.unique_channel_id == channel_id;
            });

            if (audio_set != std::end(audio_set_result))
            {
                result.append(fmt::format("                    \"volume_control_volume\": \"{}\",\n", (*audio_set).volume_set.volume));
                result.append(fmt::format("                    \"volume_control_error\": \"{}\",\n", (*audio_set).volume_control_error));
                result.append(fmt::format("                    \"volume_control_error_percent\": \"{}\"\n", (*audio_set).volume_control_error_percent));
            }

            return result;
        },
        wrapping_object,
        tabs);
}

std::string to_json(const args& args, const search_result& result, const std::vector<audio_device_unique_volume_set>& audio_set_result, bool volume_control_return_value)
{
    std::string s;
    s += "{\n";
    s += "    \"audio_devices\": [\n";
    size_t i = 0;
    for (const auto& d : result.devices)
    {
        s += "        {\n";
        s += to_json(d.first, audio_set_result, false, 2);
        s += ",\n";
        s += to_json(d.second, false, 2);
        s += "\n";
        s += "        }";
        if ((i + 1) < result.devices.size())
        {
            s.append(",");
        }
        s.append("\n");
        i++;
    }

    s += "    ],\n";
    s += "    \"serial_ports\": [\n";

    size_t j = 0;
    for (const auto& p : result.ports)
    {
        s += "        {\n";
        s += to_json(p.first, false, 2);
        s += ",\n";
        s += to_json(p.second, false, 2);
        s += "\n";
        s += "        }";
        if ((j + 1) < result.ports.size())
        {
            s.append(",");
        }
        s.append("\n");
        j++;
    }

    s += "    ]";

    if (args.test_volume_control)
    {
        s += ",\n";
        s += "    \"volume_control_test_result\": ";
        if (volume_control_return_value)
            s += "\"success\"";
        else
            s += "\"failure\"";
    }

    if (!args.ignore_config)
    {
        s += ",\n";
        std::string config_file = std::filesystem::absolute(args.config_file).string();
        s += "    \"config_file\": \"" + config_file + "\"\n";
    }
    else
    {
        s += "\n";
    }

    s += "}";

    return s;
}

// **************************************************************** //
//                                                                  //
// AUDIO DEVICES                                                    //
//                                                                  //
// **************************************************************** //

std::vector<audio_device_info> get_audio_devices(const audio_device_filter& m);
bool match_audio_device(const audio_device_info& d, const audio_device_filter& m);
bool match_device(const device_description& p, const audio_device_filter& m);
bool try_get_audio_device_channel(const audio_device_info& audio_device, const std::string& control_name, audio_device_channel_id channel_id, audio_device_type channel_type, audio_device_channel& channel);

std::vector<audio_device_info> get_audio_devices(const audio_device_filter& m)
{
    std::vector<audio_device_info> matchedDevices;
    std::vector<audio_device_info> devices = get_audio_devices();
    for (const auto& d : devices)
    {
        if (match_audio_device(d, m))
        {
            matchedDevices.emplace_back(std::move(d));
        }
    }
    return matchedDevices;
}

bool match_audio_device(const audio_device_info& d, const audio_device_filter& m)
{
    std::string device_name = to_lower(d.name);
    std::string device_name_filter = to_lower(m.name_filter);
    std::string device_desc = to_lower(d.description);
    std::string device_desc_filter = to_lower(m.desc_filter);
    std::string device_stream_name = to_lower(d.stream_name);
    std::string device_stream_name_filter = to_lower(m.stream_name_filter);

    if ((!device_name_filter.empty() && device_name.find(device_name_filter) == std::string::npos) ||
        (!device_desc_filter.empty() && device_desc.find(device_desc_filter) == std::string::npos) ||
        (!device_stream_name_filter.empty() && device_stream_name.find(device_stream_name_filter) == std::string::npos))
    {
        return false;
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
    if (!m.playback_and_capture && !m.playback_or_capture && (m.playback_only || m.capture_only))
    {
        if (m.playback_only && (!enum_device_type_has_flag(d.type, audio_device_type::playback) || enum_device_type_has_flag(d.type, audio_device_type::capture)))
        {
            return false;
        }
        if (m.capture_only && (!enum_device_type_has_flag(d.type, audio_device_type::capture) || enum_device_type_has_flag(d.type, audio_device_type::playback)))
        {
            return false;
        }
    }

    return true;
}

bool match_device(const device_description& p, const audio_device_filter& m)
{
    if (m.bus != -1 && m.bus != p.bus_number)
        return false;
    if (m.device != -1 && m.device != p.device_number)
        return false;
    if (m.topology != -1 && m.topology != p.topology_depth)
        return false;
    if (!m.path.empty() && m.path != p.hw_path)
        return false;

    return true;
}

bool try_get_audio_device_channel(const audio_device_info& audio_device, const audio_device_volume_control& control, const audio_device_channel& channel, audio_device_channel& result)
{
    audio_device_volume_info volume;
    try_get_audio_device_volume(audio_device, volume);

    for (auto& volume_control : volume.controls)
    {
        if (volume_control.name != control.name)
        {
            continue;
        }

        for (auto& control_channel : control.channels)
        {
            if (control_channel.id != channel.id || control_channel.type != channel.type)
            {
                continue;
            }

            result = control_channel;
            return true;
        }
    }

    return false;
}

bool try_get_audio_device_channel(const audio_device_info& audio_device, const std::string& control_name, audio_device_channel_id channel_id, audio_device_type channel_type, audio_device_channel& result)
{
    audio_device_volume_info volume;
    try_get_audio_device_volume(audio_device, volume);

    for (auto& control : volume.controls)
    {
        if (control.name != control_name)
        {
            continue;
        }

        for (auto& channel : control.channels)
        {
            if (channel.id != channel_id || channel.type != channel_type)
            {
                continue;
            }

            result = channel;
            return true;
        }
    }

    return false;
}

// **************************************************************** //
//                                                                  //
// SERIAL PORTS                                                     //
//                                                                  //
// **************************************************************** //

std::vector<serial_port> get_serial_ports(const serial_port_filter& m);
bool match_port(const serial_port& p, const serial_port_filter& m);
bool match_device(const device_description& p, const serial_port_filter& m);

std::vector<serial_port> get_serial_ports(const serial_port_filter& m)
{
    std::vector<serial_port> matchedPorts;
    std::vector<serial_port> ports = get_serial_ports();
    for (serial_port p : ports)
    {
        if (match_port(p, m))
            matchedPorts.push_back(p);
    }
    return matchedPorts;
}

bool match_port(const serial_port& p, const serial_port_filter& m)
{
    std::string port_name = to_lower(p.name);
    std::string port_name_filter = to_lower(m.name_filter);
    std::string port_desc = to_lower(p.description);
    std::string port_desc_filter = to_lower(m.description_filter);
    std::string port_mfd = to_lower(p.manufacturer);
    std::string port_mfd_filter = to_lower(m.manufacturer_filter);
    std::string port_serial = to_lower(p.device_serial_number);
    std::string port_serial_filter = to_lower(m.device_serial_number);

    if (!port_name_filter.empty() && port_name.find(port_name_filter) == std::string::npos)
        return false;
    if (!port_desc_filter.empty() && port_desc.find(port_desc_filter) == std::string::npos)
        return false;
    if (!port_mfd_filter.empty() && port_mfd.find(port_mfd_filter) == std::string::npos)
        return false;
    if (!port_serial_filter.empty() && port_serial.find(port_serial_filter) == std::string::npos)
        return false;

    return true;
}

bool match_device(const device_description& p, const serial_port_filter& m)
{
    if (m.bus != -1 && m.bus != p.bus_number)
        return false;
    if (m.device != -1 && m.device != p.device_number)
        return false;
    if (m.topology != -1 && m.topology != p.topology_depth)
        return false;
    if (!m.path.empty() && m.path != p.hw_path)
        return false;

    return true;
}

// **************************************************************** //
//                                                                  //
// SEARCH                                                           //
//                                                                  //
// **************************************************************** //

std::vector<std::pair<audio_device_info, device_description>> filter_audio_devices(const args& args, const std::vector<audio_device_info>& devices);
std::vector<std::pair<serial_port, device_description>> filter_serial_ports(const args& args, const std::vector<serial_port>& ports);
std::vector<audio_device_info> get_sibling_audio_devices(const std::vector<std::pair<serial_port, device_description>>& ports);
std::vector<serial_port> get_sibling_serial_ports(const std::vector<std::pair<audio_device_info, device_description>>& devices);
std::vector<std::pair<audio_device_volume_info, device_description>> map_device_to_volume(const std::vector<std::pair<audio_device_info, device_description>>& devices);
search_result search(const args& args);
bool has_audio_device_description_filter(const args& args);
bool has_serial_port_description_filter(const args& args);
std::vector<audio_device_unique_volume_set> generate_unique_volume_set(const args& args, const search_result& result);
std::string create_unique_channel_id(const audio_device_info& device, const audio_device_volume_control& control, const audio_device_channel& channel);
audio_device_unique_volume_set create_unique_volume_set_object(const audio_device_volume_info& volume, const audio_device_volume_control& control, const audio_device_channel& channel, const audio_device_volume_set& volume_set);

std::vector<std::pair<audio_device_info, device_description>> filter_audio_devices(const args& args, const std::vector<audio_device_info>& devices)
{
    std::vector<std::pair<audio_device_info, device_description>> audio_devices;
    for (audio_device_info d : devices)
    {
        if (!match_audio_device(d, args.audio_filter))
            continue;
        device_description desc;
        if (try_get_device_description(d, desc))
        {
            if (!match_device(desc, args.audio_filter))
                continue;
        }
        else if (has_audio_device_description_filter(args))
            continue;
        if (std::find_if(audio_devices.begin(), audio_devices.end(), [&](const auto& dev) { return dev.first.hw_id == d.hw_id; }) != audio_devices.end())
            continue;
        audio_devices.push_back(std::make_pair(d, desc));
    }
    return audio_devices;
}

std::vector<std::pair<serial_port, device_description>> filter_serial_ports(const args& args, const std::vector<serial_port>& ports)
{
    std::vector<std::pair<serial_port, device_description>> serial_ports;
    for (serial_port p : ports)
    {
        if (!match_port(p, args.port_filter))
            continue;
        device_description desc;
        if (try_get_device_description(p, desc))
        {
            if (!match_device(desc, args.port_filter))
                continue;
        }
        else if (has_serial_port_description_filter(args))
            continue;
        if (std::find_if(serial_ports.begin(), serial_ports.end(), [&](const auto& port) { return port.first.name == p.name; }) != serial_ports.end())
            continue;
        serial_ports.push_back(std::make_pair(p, desc));
    }
    return serial_ports;
}

std::vector<audio_device_info> get_sibling_audio_devices(const std::vector<std::pair<serial_port, device_description>>& ports)
{
    std::vector<audio_device_info> devices;
    for (const auto& p : ports)
    {
        for (const auto& d : get_sibling_audio_devices(p.second))
        {
            for (const auto& a : get_audio_devices(d))
            {
                if (std::find_if(devices.begin(), devices.end(), [&](const auto& dev) { return dev.hw_id == a.hw_id; }) != devices.end())
                    continue;
                devices.push_back(a);
            }
        }
    }
    return devices;
}

std::vector<serial_port> get_sibling_serial_ports(const std::vector<std::pair<audio_device_info, device_description>>& devices)
{
    std::vector<serial_port> ports;
    for (const auto& a : devices)
    {
        for (const auto& d : get_sibling_serial_ports(a.second))
        {
            serial_port p;
            if (try_get_serial_port(d, p))
            {
                if (std::find_if(ports.begin(), ports.end(), [&](const auto& port) { return port.name == p.name; }) != ports.end())
                    continue;
                ports.push_back(p);
            }
        }
    }
    return ports;
}

std::vector<std::pair<audio_device_volume_info, device_description>> map_device_to_volume(const std::vector<std::pair<audio_device_info, device_description>>& devices)
{
    std::vector<std::pair<audio_device_volume_info, device_description>> devices_volumes;
    for (const auto& device : devices)
    {
        audio_device_volume_info device_volume;
        try_get_audio_device_volume(device.first, device_volume);
        devices_volumes.push_back(std::make_pair(device_volume, device.second));
    }
    return devices_volumes;
}

search_result search(const args& args)
{
    search_result result;
    if (args.search_mode == search_mode::independent)
    {
        result.devices = map_device_to_volume(filter_audio_devices(args, get_audio_devices()));
        result.ports = filter_serial_ports(args, get_serial_ports());
    }
    else if (args.search_mode == search_mode::port_siblings)
    {
        result.ports = filter_serial_ports(args, get_serial_ports());
        result.devices = map_device_to_volume(filter_audio_devices(args, get_sibling_audio_devices(result.ports)));
    }
    else if (args.search_mode == search_mode::audio_siblings)
    {
        auto devices = filter_audio_devices(args, get_audio_devices());
        result.devices = map_device_to_volume(devices);
        result.ports = filter_serial_ports(args, get_sibling_serial_ports(devices));
    }
    return result;
}

bool has_audio_device_description_filter(const args& args)
{
    return (args.audio_filter.bus != -1 ||
        args.audio_filter.device != -1 ||
        !args.audio_filter.path.empty() ||
        args.audio_filter.topology != -1);
}

bool has_serial_port_description_filter(const args& args)
{
    return (args.port_filter.bus != -1 ||
        args.port_filter.device != -1 ||
        !args.port_filter.path.empty() ||
        args.port_filter.topology != -1);
}

std::vector<audio_device_unique_volume_set> generate_unique_volume_set(const args& args, const search_result& result)
{
    std::map<std::string, audio_device_unique_volume_set> visitors;

    for (auto& dd : result.devices)
    {
        audio_device_volume_info volume = dd.first;

        for (auto& volume_set : args.volume_set)
        {
            if (volume_set.volume == -1)
            {
                continue;
            }

            for (auto& control : volume.controls)
            {
                // if the control name is set and it does not match, skip it
                if (volume_set.control_name.size() > 0 && volume_set.control_name != control.name)
                {
                    continue;
                }

                for (auto& channel : control.channels)
                {
                    // if the channel type is set and it does not match, skip it
                    if (volume_set.audio_channel_type != audio_device_type::uknown && channel.type != volume_set.audio_channel_type)
                    {
                        continue;
                    }

                    if (volume_set.audio_channels.size() == 0)
                    {
                        audio_device_unique_volume_set visitor = create_unique_volume_set_object(volume, control, channel, volume_set);

                        if (!visitors.contains(visitor.unique_channel_id) || visitors[visitor.unique_channel_id].property_set_count < visitor.property_set_count)
                            visitors[visitor.unique_channel_id] = visitor;

                        continue;
                    }

                    for (auto& channel_set : volume_set.audio_channels)
                    {
                        if (channel_set != channel.id)
                        {
                            continue;
                        }

                        audio_device_unique_volume_set visitor = create_unique_volume_set_object(volume, control, channel, volume_set);

                        if (!visitors.contains(visitor.unique_channel_id) || visitors[visitor.unique_channel_id].property_set_count < visitor.property_set_count)
                            visitors[visitor.unique_channel_id] = visitor;
                    }
                }
            }
        }
    }

    std::vector<audio_device_unique_volume_set> visitors_vect;
    for (const auto& v : visitors)
    {
        visitors_vect.push_back(v.second);
    }

    return visitors_vect;
}

audio_device_unique_volume_set create_unique_volume_set_object(const audio_device_volume_info& volume, const audio_device_volume_control& control, const audio_device_channel& channel, const audio_device_volume_set& volume_set)
{
    audio_device_unique_volume_set visitor;
    visitor.volume = volume;
    visitor.control = control;
    visitor.channel = channel;
    visitor.volume_set = volume_set;
    visitor.unique_channel_id = create_unique_channel_id(volume.audio_device, control, channel);
    if (volume_set.control_name.size() > 0)
        visitor.property_set_count++;
    if (volume_set.audio_channels.size() > 0)
        visitor.property_set_count++;
    return visitor;
}

std::string create_unique_channel_id(const audio_device_info& device, const audio_device_volume_control& control, const audio_device_channel& channel)
{
    return fmt::format("{},{},{},{}", device.hw_id, control.name, to_string(channel.id), to_string(channel.type));
}

// **************************************************************** //
//                                                                  //
// COMMAND LINE                                                     //
//                                                                  //
// **************************************************************** //

bool has_volume_control_options(int argc, char* argv[]);
bool try_parse_command_line(int argc, char* argv[], args& args);

bool has_volume_control_options(int argc, char* argv[])
{
    for (int i = 0; i < argc; i++)
    {
        std::string arg = std::string(argv[i]);
        if (arg == "--audio.volume" || arg == "--audio.control" ||
            arg == "--audio.channels" || arg == "--audio.channel-type")
            return true;
    }
    return false;
}

bool try_parse_command_line(int argc, char* argv[], args& args)
{
    std::map<std::string, option_handler> command_map =
    {
        { "version", {"v,version", false, nullptr, [&](const cxxopts::ParseResult& result) { args.show_version = true; }}},
        { "help", {"h,help", false, nullptr, [&](const cxxopts::ParseResult& result) { args.help = true; }}},
        { "disable-colors", {"disable-colors", false, nullptr, [&](const cxxopts::ParseResult& result) { args.disable_colors = result["disable-colors"].as<bool>(); }}},
        { "disable-file-write", {"disable-file-write", false, nullptr, [&](const cxxopts::ParseResult& result) { args.disable_write_file = result["disable-file-write"].as<bool>(); }}},
        { "ignore-config", {"ignore-config", false, nullptr, [&](const cxxopts::ParseResult& result) { args.ignore_config = result["ignore-config"].as<bool>(); }}},
        { "list-properties", {"p,list-properties", false, nullptr, [&](const cxxopts::ParseResult& result) { args.list_properties = result["list-properties"].as<bool>(); }}},
        { "use-json", {"j,use-json", false, nullptr, [&](const cxxopts::ParseResult& result) { args.use_json = result["use-json"].as<bool>(); }}},
        { "search-mode", {"s,search-mode", true, cxxopts::value<std::string>()->default_value("independent"), [&](const cxxopts::ParseResult& result) { try_parse_search_mode(result["search-mode"].as<std::string>(), args.search_mode); }}},
        { "included-devices", {"i,included-devices", true, cxxopts::value<std::string>()->default_value("all"), [&](const cxxopts::ParseResult& result) { try_parse_included_devices(result["included-devices"].as<std::string>(), args.included_devices); }}},
        { "verbose", {"verbose", true, cxxopts::value<bool>()->default_value("true"), [&](const cxxopts::ParseResult& result) { args.verbose = result["verbose"].as<bool>(); }}},
        { "no-stdout", {"no-stdout", false, nullptr, [&](const cxxopts::ParseResult& result) { args.no_stdout = true; }}},
        { "no-verbose", {"no-verbose", false, nullptr, [&](const cxxopts::ParseResult& result) { args.verbose = false; }}},
        { "config-file", {"c,config-file", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.config_file = result["config-file"].as<std::string>(); }}},
        { "output-file", {"o,output-file", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.output_file = result["output-file"].as<std::string>(); }}},
        { "audio.desc", {"audio.desc", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.audio_filter.desc_filter = result["audio.desc"].as<std::string>(); }}},
        { "audio.name", {"audio.name", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.audio_filter.name_filter = result["audio.name"].as<std::string>(); }}},
        { "audio.stream-name", {"audio.stream-name", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.audio_filter.stream_name_filter = result["audio.stream-name"].as<std::string>(); }}},
        { "audio.type", {"audio.type", true, cxxopts::value<std::string>()->default_value("playback|capture"), [&](const cxxopts::ParseResult& result) { parse_audio_device_type(result["audio.type"].as<std::string>(), args); }}},
        { "audio.bus", {"audio.bus", true, cxxopts::value<int>(), [&](const cxxopts::ParseResult& result) { args.audio_filter.bus = result["audio.bus"].as<int>(); }}},
        { "audio.device", {"audio.device", true, cxxopts::value<int>(), [&](const cxxopts::ParseResult& result) { args.audio_filter.device = result["audio.device"].as<int>(); }}},
        { "audio.topology", {"audio.topology", true, cxxopts::value<int>(), [&](const cxxopts::ParseResult& result) { args.audio_filter.topology = result["audio.topology"].as<int>(); }}},
        { "audio.path", {"audio.path", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.audio_filter.path = result["audio.path"].as<std::string>(); }}},     
        { "audio.control", {"audio.control", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.volume_set[0].control_name = result["audio.control"].as<std::string>(); }}},
        { "audio.channels", {"audio.channels", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { try_parse_channels(result["audio.channels"].as<std::string>(), args.volume_set[0].audio_channels); }}},
        { "audio.volume", {"audio.volume", true, cxxopts::value<int>(), [&](const cxxopts::ParseResult& result) { args.volume_set[0].volume = result["audio.volume"].as<int>(); }}},
        { "audio.channel-type", {"audio.channel-type", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { try_parse_audio_device_type(result["audio.channel-type"].as<std::string>(), args.volume_set[0].audio_channel_type); }}},
        { "audio.disable-volume-control", {"audio.disable-volume-control", false, nullptr, [&](const cxxopts::ParseResult& result) { args.disable_volume_control = result["audio.disable-volume-control"].as<bool>(); }}},
        { "no-volume-control", {"no-volume-control", false, nullptr, [&](const cxxopts::ParseResult& result) { args.disable_volume_control = result["no-volume-control"].as<bool>(); }}},
        { "test-volume-control", {"test-volume-control", false, nullptr, [&](const cxxopts::ParseResult& result) { args.test_volume_control = result["test-volume-control"].as<bool>(); }}},
        { "probe-volume-control", {"probe-volume-control", false, nullptr, [&](const cxxopts::ParseResult& result) { args.probe_volume_control = result["probe-volume-control"].as<bool>(); }}},
        { "port.name", {"port.name", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.port_filter.name_filter = result["port.name"].as<std::string>(); }}},
        { "port.desc", {"port.desc", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.port_filter.description_filter = result["port.desc"].as<std::string>(); }}},
        { "port.bus", {"port.bus", true, cxxopts::value<int>(), [&](const cxxopts::ParseResult& result) { try_parse_number(result["port.bus"].as<std::string>(), args.port_filter.bus); }}},
        { "port.device", {"port.device", true, cxxopts::value<int>(), [&](const cxxopts::ParseResult& result) { try_parse_number(result["port.device"].as<std::string>(), args.port_filter.device); }}},
        { "port.topology", {"port.topology", true, cxxopts::value<int>(), [&](const cxxopts::ParseResult& result) { try_parse_number(result["port.topology"].as<std::string>(), args.port_filter.topology); }}},
        { "port.path", {"port.path", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.port_filter.path = result["port.path"].as<std::string>(); }}},
        { "port.serial", {"port.serial", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.port_filter.device_serial_number = result["port.serial"].as<std::string>(); }}},
        { "port.mfn", {"port.mfn", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.port_filter.manufacturer_filter = result["port.mfn"].as<std::string>(); }}},
        { "direwolf-config", {"direwolf-config", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.direwolf_output_file = result["direwolf-config"].as<std::string>(); }}},
        { "direwolf.agwport", {"direwolf.agwport", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { try_parse_number(result["direwolf.agwport"].as<std::string>(), args.direwolf_agwport); }}},
        { "direwolf.kissport", {"direwolf.kissport", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { try_parse_number(result["direwolf.kissport"].as<std::string>(), args.direwolf_kissport); }}},
        { "direwolf.callsign", {"direwolf.callsign", true, cxxopts::value<std::string>(), [&](const cxxopts::ParseResult& result) { args.direwolf_callsign = result["direwolf.callsign"].as<std::string>(); }}}
    };

    if (has_volume_control_options(argc, argv))
    {
        args.volume_set.push_back(audio_device_volume_set{});
    }

    cxxopts::Options options("", "");

    cxxopts::OptionAdder option_group = options.allow_unrecognised_options().add_options();

    for (const auto& command : command_map)
    {
        if (command.second.type != nullptr)
        {
            option_group(command.second.command_line_arg_name, "", command.second.type);
        }
        else
        {
            option_group(command.second.command_line_arg_name, "");
        }
    }

    cxxopts::ParseResult result;

    try
    {
        result = options.parse(argc, argv);
    }
    catch (const cxxopts::exceptions::incorrect_argument_type& e)
    {
        args.command_line_error = fmt::format("Error parsing command line: {}\n\n", e.what());
        args.command_line_has_errors = true;
        return false;
    }
    catch (const std::exception& e)
    {
        args.command_line_error = "Error parsing command line.";
        args.command_line_has_errors = true;
        return false;
    }

    for (const auto& arg : result.arguments())
    {
        args.command_line_args.insert({ arg.key(), arg.value() });
    }

    std::vector<std::string> unmatched = result.unmatched();
    if (unmatched.size() > 0)
    {
        args.command_line_error = fmt::format("Error parsing command line: {}\n\n", unmatched[0]);
        args.command_line_has_errors = true;
        return false;
    }

    for (const auto& command : command_map)
    {
        const std::string& key = command.first;
        const std::function<void(const cxxopts::ParseResult&)>& handler = command.second.read_handler;
        if (result.count(key) > 0)
        {
            handler(result);
        }
    }

    return true;
}

// **************************************************************** //
//                                                                  //
// SETTINGS                                                         //
//                                                                  //
// **************************************************************** //

void read_settings(args& args);
void parse_top_level_settings(args& args, const nlohmann::json& j);
void parse_search_criteria(args& args, const nlohmann::json& j);
void parse_volume_control(args& args, const nlohmann::json& j);

void read_settings(args& args)
{
    if (args.ignore_config)
    {
        return;
    }

    std::string config_file = args.config_file;

    // Read settings from current executable directory if present.

    if (args.config_file.empty())
    {
        config_file = std::filesystem::current_path() / "config.json";
    }

    if (!std::filesystem::exists(config_file))
    {
        return;
    }

    std::ifstream i;
    try
    {
        i.open(config_file);
    }
    catch (std::ios_base::failure&)
    {
        return;
    }

    if (!i.is_open())
    {
        return;
    }

    nlohmann::json j;

    try
    {
        j = nlohmann::json::parse(i, nullptr, true, /*ignore comments*/ true);
    }
    catch (nlohmann::json::parse_error&)
    {
        return;
    }
    catch (nlohmann::json::type_error&)
    {
        return;
    }

    parse_top_level_settings(args, j);
    parse_search_criteria(args, j);    
    parse_volume_control(args, j);    
}

void parse_top_level_settings(args& args, const nlohmann::json& j)
{
    if (!args.command_line_args.contains("search-mode"))
        try_parse_search_mode(j.value("search_mode", ""), args.search_mode);
    if (!args.command_line_args.contains("use-json"))
        try_parse_bool(j.value("use_json", ""), args.use_json);
    if (!args.command_line_args.contains("list-properties"))
        try_parse_bool(j.value("list_properties", ""), args.list_properties);
    if (j.contains("output_file") && !args.command_line_args.contains("output-file"))
        {
            args.disable_write_file = false;
            args.output_file = j["output_file"];
            args.output_file = get_full_path(args.output_file);
        }
    if (!args.command_line_args.contains("included-devices"))
        try_parse_included_devices(j.value("included_devices", ""), args.included_devices);
}

void parse_search_criteria(args& args, const nlohmann::json& j)
{
    if (j.contains("search_criteria"))
    {
        nlohmann::json search_criteria = j["search_criteria"];
        if (search_criteria.contains("audio"))
        {
            nlohmann::json audio_match = search_criteria["audio"];
            if (!args.command_line_args.contains("audio.name"))
                args.audio_filter.name_filter = audio_match.value("name", "");
            if (!args.command_line_args.contains("audio.stream-name"))
                args.audio_filter.stream_name_filter = audio_match.value("stream_name", "");
            if (!args.command_line_args.contains("audio.desc"))
                args.audio_filter.desc_filter = audio_match.value("desc", "");
            if (!args.command_line_args.contains("audio.type"))
                parse_audio_device_type(audio_match.value("type", ""), args);
            if (!args.command_line_args.contains("audio.bus"))
                try_parse_number(audio_match.value("bus", ""), args.audio_filter.bus);
            if (!args.command_line_args.contains("audio.device"))
                try_parse_number(audio_match.value("device", ""), args.audio_filter.device);
            if (!args.command_line_args.contains("audio.topology"))
                try_parse_number(audio_match.value("topology_depth", ""), args.audio_filter.topology);
            if (!args.command_line_args.contains("audio.path"))
                args.audio_filter.path = audio_match.value("path", "");
        }
        if (search_criteria.contains("port"))
        {
            nlohmann::json port_match = search_criteria["port"];
            if (!args.command_line_args.contains("port.name"))
                args.port_filter.name_filter = port_match.value("name", "");
            if (!args.command_line_args.contains("port.desc"))
                args.port_filter.description_filter = port_match.value("desc", "");
            if (!args.command_line_args.contains("port.bus"))
                try_parse_number(port_match.value("bus", ""), args.port_filter.bus);
            if (!args.command_line_args.contains("port.device"))
                try_parse_number(port_match.value("device", ""), args.port_filter.device);
            if (!args.command_line_args.contains("port.toplogy"))
                try_parse_number(port_match.value("topology_depth", ""), args.port_filter.topology);
            if (!args.command_line_args.contains("port.path"))
                args.port_filter.path = port_match.value("path", "");
            if (!args.command_line_args.contains("port.serial"))
                args.port_filter.device_serial_number = port_match.value("serial", "");
        }
    }
}

void parse_volume_control(args& args, const nlohmann::json& control, const std::string& control_name, const std::string& property_name, const std::string& err_property_name, audio_device_type type, audio_device_volume_set& volume_set)
{
    if (!control.contains(property_name))
    {        
        return;
    }

    try_parse_number(control.value(property_name, ""), volume_set.volume);
    volume_set.audio_channel_type = type;
    if (control.contains(err_property_name))
        {
        try_parse_number(control.value(err_property_name, ""), volume_set.volume_max_error);
                }
    volume_set.control_name = control_name;
    args.volume_set.push_back(volume_set);
            }

void parse_volume_control(args& args, const nlohmann::json& control, const std::string& control_name, const std::string& property_name, const std::string&  err_property_name, audio_device_type type)
            {
    audio_device_volume_set volume_set;
    parse_volume_control(args, control, control_name, property_name, err_property_name, type, volume_set);
                }

void parse_volume_control(args& args, const nlohmann::json& control, const std::string& property_name, const std::string& err_property_name, audio_device_type type, audio_device_volume_set &volume_set)
            {
    std::string control_name;
                    if (control.contains("name"))
                    {
        control_name = control["name"];
                    }
    parse_volume_control(args, control, control_name, property_name, err_property_name, type, volume_set);
                    }

void parse_volume_control(args& args, const nlohmann::json& control, const std::string& property_name, const std::string& err_property_name, audio_device_type type)
                    {
    audio_device_volume_set volume_set;
    parse_volume_control(args, control, property_name, err_property_name, type, volume_set);
                    }   

void parse_volume_control(args& args, const nlohmann::json& control, const nlohmann::json& channel, const std::string& property_name, const std::string& err_property_name, audio_device_type type)
                    {
    std::string control_name;
    if (control.contains("name"))
                        {
        control_name = control["name"];
                        }
    parse_volume_control(args, channel, control_name, property_name, err_property_name, type);
    std::string channel_name;
                        if (channel.contains("name"))
                        {
        channel_name = channel["name"];
        audio_device_channel_id channel_id;
        if (try_parse_audio_device_channel_display_name(channel_name, channel_id))
                        {
            args.volume_set.back().audio_channels.push_back(channel_id);
                        }
    }
                        }

void parse_volume_control(args& args, const nlohmann::json& j)
                        {
    if (args.volume_set.size() > 0)
                            {
        return;
                            }                     
    
    if (!j.contains("volume_control"))
                            {
        return;
                        }   

    nlohmann::json volume_control = j["volume_control"];
      
    parse_volume_control(args, volume_control, "capture_value_percent", "capture_value_test_max_error", audio_device_type::capture);
    parse_volume_control(args, volume_control, "playback_value_percent", "playback_value_test_max_error", audio_device_type::playback);

    if (volume_control.contains("controls"))
                        {
        nlohmann::json controls = volume_control["controls"];
        for (const nlohmann::json& control : controls)
                            {
            parse_volume_control(args, control, "capture_value_percent", "capture_value_test_max_error", audio_device_type::capture);
            parse_volume_control(args, control, "playback_value_percent", "playback_value_test_max_error", audio_device_type::playback);

            if (control.contains("channels"))
            {
            nlohmann::json channels = control["channels"];
            for (const nlohmann::json& channel : channels)
                            {
                parse_volume_control(args, control, channel, "capture_value_percent", "capture_value_test_max_error", audio_device_type::capture);
                parse_volume_control(args, control, channel, "playback_value_percent", "playback_value_test_max_error", audio_device_type::playback);
            }
        }        
    }
}
}

// **************************************************************** //
//                                                                  //
// MAIN AND HIGH LEVEL FUNCTIONS                                    //
//                                                                  //
// **************************************************************** //

int main(int argc, char* argv[]);
void print_usage();
void print_stdout(const args& args, const search_result& result);
void print_to_file(const args& args, const std::string& json);
void adjust_volume(const args& args, const audio_device_volume_info& volume, const audio_device_volume_control& control, const audio_device_channel& channel, const audio_device_volume_set& volume_set);
std::vector<audio_device_unique_volume_set> adjust_volume(const args& args, search_result& result);
bool test_volume_control(const args& args, const search_result& result);
void print_adjust_volume_results(const args& args, const std::vector<audio_device_unique_volume_set>& audio_set_result);
void update_devices_volume(search_result& result);
void print(const args& args, const search_result& result, int volume_control_return_value, const std::vector<audio_device_unique_volume_set>& audio_set_result);
int process_devices(const args& args);
void print_version();
bool generate_direwolf_output_file(const args& args, const search_result& result);

int main(int argc, char* argv[])
{
    args args;

    if (!try_parse_command_line(argc, argv, args))
    {
        return 1;
    }

    if (args.command_line_has_errors && !args.no_stdout)
    {
        printf("%s\n\n", args.command_line_error.c_str());
        print_usage();
        return 1;
    }

    if (args.help)
    {
        print_usage();
        return 1;
    }

    if (args.show_version)
    {
        print_version();
        return 1;
    }

    read_settings(args);

    return process_devices(args);
}

void print_usage()
{
    std::string usage =
        "find_devices - audio device and serial ports finding utility\n"
        "version "
        STRINGIFY(FIND_DEVICES_VERSION)
#ifdef GIT_HASH        
        " "
        STRINGIFY(GIT_HASH)
#endif        
        "\n"
        "(C) 2023 Ion Todirel\n"
        "\n"
        "Usage:\n"
        "    find_devices [OPTION]... \n"
        "\n"
        "Options:\n"
        "    --audio.name <name>               search filter: partial or complete name of the audio device\n"
        "    --audio.stream-name <name>        search filter: partial or complete name of the audio stream name\n"
        "    --audio.desc <description>        search filter: partial or complete description of the audio device\n"
        "    --audio.type <type>               search filter: types of audio devices to find: playback, capture, playback|capture, playback&capture:\n"
        "                                          playback - playback only\n"
        "                                          capture - capture only\n"
        "                                          \"playback|capture\" - playback or capture\n"
        "                                          \"playback&capture\" - playback and capture\n"
        "                                          all\n"
        "    --audio.bus <number>              search filter: audio device bus number\n"
        "    --audio.device <number>           search filter: audio device number\n"
        "    --audio.path <path>               search filter: audio device hardware system path\n"
        "    --audio.topology <number>         search filter: the depth of the audio device topology, in the device tree\n"
        "    --audio.control <name>            used to set a value on the audio device; this property is used to select the audio control to set\n"
        "    --audio.channels <channels>       used to set a value on the audio device; this property is used to select the audio channels to set\n"
        "    --audio.volume <volume>           used to set a value on the audio device; this property is used to set the audio volume\n"
        "                                      on all devices that match --audio.control, --audio.channels and --audio.channel-type.\n"
        "    --audio.channel-type <type>       used to set a value on the audio device; this property is used to select the channel type\n"
        "                                          playback - playback only\n"
        "                                          capture - capture only\n"
        "                                          all\n"
        "    --audio.disable-volume-control    disable setting the audio device volume\n"
        "    --port.name <name>                search filter: partial or complete name of the serial port\n"
        "    --port.desc <description>         search filter: partial or complete description of the serial port\n"
        "    --port.bus <number>               search filter: serial port bus number\n"
        "    --port.device <number>            search filter: serial port device number\n"
        "    --port.topology <number>          search filter: the depth of the serial port device topology, in the device tree\n"
        "    --port.path <path>                search filter: serial port hardware system path\n"
        "    --port.serial <serial>            search filter: partial or complete serial port device serial number\n"
        "    --port.mfn <name>                 search filter: partial or complete serial port manufacturer name\n"
        "    -v, --verbose                     enable detailed printing to stdout\n"
        "    --no-verbose                      disable detailed printing to stdout\n"
        "    --no-stdout                       don't print to stdout\n"
        "    --no-volume-control               disable setting the audio device volume\n"
        "    --probe-volume-control            tests volume control from 0 to 100 and lists all valid volume levels\n"
        "    --test-volume-control             verifies whether the audio devices matching the serarch criteria\n"
        "                                      match the specification given in the volume control\n"
        "                                      if the volume control do not match, the exit code is 1\n"
        "    -h, --help                        print help\n"
        "    -v, --version                     prints the version of this program\n"
        "    -p, --list-properties             print detailed properties for each device and serial port\n"
        "    -i, --included-devices <type>     type of devices to include in searches and in stdout or JSON:\n"
        "                                          audio - include audio devices\n"
        "                                          ports - include serial ports\n"
        "                                          all - include audio devices and serial ports\n"
        "    -s, --search-mode <mode>          how to conduct the search: \n"
        "                                          independent - look for audio devices and serial ports independently\n"
        "                                          audio-siblings - look for audio devices and find their sibling serial ports\n"
        "                                          port-siblings - look for serial ports and find their sibling audio devices\n"
        "    --output-file <file>              write results as JSON to a file\n"
        "    --json                            display JSON to stdout\n"
        "    --ignore-config                   ignore the configuration file, if a configurtion file is available or specified\n"
        "    -c, --config-file <file>          use a configuration file to configure the program\n"
        "                                      settings specified as command line args override settings present in the config file\n"
        "                                      if not specified default config file name used is \"config.json\"\n"
        "    --disable-colors                  do not print colors in stdout\n"
        "    --disable-file-write              disables writing a JSON file with the results of the search, which is the defaul\n"
        "    --test-data <file>                not yet implemented: fake the data as if it came from the system, for testing purposes\n"
        "    -t, --test-devices                not yet implemented: test each device hardware that we find\n"
        "                                      if hardware test fails, removes it from the search results list\n"
        "    --direwolf-config <file>          generate a direwolf configuration file\n"
        "    --direwolf.agwport <port>         the AGW port in the direwolf configuration\n"
        "    --direwolf.kissport <port>        the KISS port in the direwolf configuration\n"
        "    --direwolf.callsign <port>        the callsign in the direwolf configuration, NOCALL if not specified\n"
        "\n"
        "Return:\n"
        "    0 - success, audio devices or serial ports are found matching the search criteria\n"
        "    1 - if the command line arguments are incorrect, or if called with --help\n"
        "    1 - if no devices are found, or no devices are matching the search criteria\n"
        "\n"
        "Examples:\n"
        "    find_devices --audio.name \"USB Audio\" --audio.desc \"Texas Instruments\" --no-verbose\n"
        "    find_devices --audio.desc \"C-Media Electronics Inc.\" -s audio-siblings -i all\n"
        "    find_devices --audio.desc \"C-Media\" --port.desc \"CP2102N\" -s port-siblings -i all \n"
        "    find_devices --audio.bus=2 --audio.device=48 -s audio-siblings -i audio \n"
        "    find_devices --audio.type \"playback&capture\"\n"
        "    find_devices -h\n"
        "    find_devices -j -o output.json\n"
        "    find_devices -c digirig_config.json\n"
        "    find_devices --audio.control Speakers --audio.channels=\"Front Left, Front Center\" --audio.volume 60 --audio.channel-type=capture\n"
        "    find_devices --audio.control Speakers --audio.channels=\"Front Left\" --audio.volume 80 --audio.channel-type=playback\n"
        "    find_devices --audio.volume 50 --audio.channel-type=playback\n"
        "    find_devices --audio.control Speakers --audio.channels=\"Front Left, Front Center\" --audio.volume 50\n"
        "\n"
        "Defaults:\n"
        "    --verbose\n"
        "    --audio.type all\n"
        "    -i all\n"
        "    -s independent\n"
        "\n";
    printf("%s", usage.c_str());
}

void print_stdout(const args& args, const search_result& result)
{
    if (args.no_stdout)
    {
        return;
    }

    if (args.included_devices == included_devices::all || args.included_devices == included_devices::audio)
    {
        if (args.verbose)
        {
            print(!args.disable_colors, fmt::emphasis::bold, "\nFound audio devices:\n\n");
        }

        // Current formatting settings support audio device count of up to 999
        // and up to 999 serial ports before breaking formatting

        size_t i = 1;
        for (const auto& d : result.devices)
        {
            if (!args.verbose)
                printf("%s\n", d.first.audio_device.plughw_id.c_str());
            else
            {
                print(!args.disable_colors, fmt::emphasis::bold, "{:>4})", i);
                print(!args.disable_colors, fmt::emphasis::bold | fg(fmt::color::chartreuse), " {}", d.first.audio_device.hw_id);
                fmt::print(": ");
                print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::cornflower_blue), "{}", d.first.audio_device.name);
                fmt::print(" - ");
                print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::chocolate), "{}", d.first.audio_device.description);
                fmt::println("");

                if (args.list_properties)
                {
                    fmt::println("");
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "hwid");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.audio_device.hw_id);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "plughwid");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.audio_device.plughw_id);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "name");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.audio_device.name);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "description");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.audio_device.description);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "stream name");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.audio_device.stream_name);

                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "volume controls");
                    for (size_t k = 0; k < d.first.controls.size(); k++)
                    {
                        for (size_t m = 0; m < d.first.controls[k].channels.size(); m++)
                        {
                            print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}%", d.first.controls[k].channels[m].volume_percent);
                            if ((m + 1) < d.first.controls[k].channels.size())
                                print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), ", ");
                        }
                        if ((k + 1) < d.first.controls.size())
                            print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), ", ");
                    }
                    fmt::print("\n");

                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "bus");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.bus_number);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "device");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.device_number);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "product");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.product);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "idProduct");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.id_product);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "idVendor");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.id_vendor);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "path");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.path);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "hardware path");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.hw_path);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "depth");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.topology_depth);
                    if (i < result.devices.size())
                        fmt::println("");
                }
            }
            i++;
        }
    }

    if (args.included_devices == included_devices::all || args.included_devices == included_devices::ports)
    {
        if (args.verbose)
        {
            print(!args.disable_colors, fmt::emphasis::bold, "\nFound serial ports:\n\n");
        }

        size_t j = 1;
        for (const auto& p : result.ports)
        {
            if (!args.verbose)
                printf("%s\n", p.first.name.c_str());
            else
            {
                print(!args.disable_colors, fmt::emphasis::bold, "{:>4})", j);
                print(!args.disable_colors, fmt::emphasis::bold | fg(fmt::color::chartreuse), " {}", p.first.name);
                fmt::print(": ");
                print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::cornflower_blue), "{}", p.first.manufacturer);
                fmt::print(" - ");
                print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::chocolate), "{}", p.first.description);
                fmt::println("");

                if (args.list_properties)
                {
                    fmt::println("");
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "name");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.first.name);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "manufacturer");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.first.manufacturer);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "description");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.first.description);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "sn");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.first.device_serial_number);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "bus");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.bus_number);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "device");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.device_number);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "product");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.product);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "idProduct");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.id_product);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "idVendor");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.id_vendor);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "path");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.path);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "hardware path");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.hw_path);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "depth");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.topology_depth);
                    if (j < result.ports.size())
                        fmt::println("");
                }
            }
            j++;
        }
    }

    printf("\n");
}

void print_to_file(const args& args, const std::string& json)
{
    if (args.disable_write_file)
    {
        return;
    }

    std::string file_name = args.output_file;

    if (args.output_file.empty())
    {
        try_find_new_filename(std::filesystem::current_path() / "output.json", file_name);
    }

    if (std::filesystem::exists(file_name))
        std::filesystem::remove(file_name);

    write_line_to_file(file_name, json);

    if (args.verbose && !args.use_json && !args.no_stdout)
    {
        print(!args.disable_colors, fmt::emphasis::bold, "Wrote to file: ");
        print(!args.disable_colors, fg(fmt::color::red), "{}\n\n", file_name);
    }
}

void adjust_volume(const args& args, const audio_device_volume_info& volume, const audio_device_volume_control& control, const audio_device_channel& channel, const audio_device_volume_set& volume_set)
{
    audio_device_channel updated_channel = channel;

    updated_channel.volume_percent = volume_set.volume;

    try_set_audio_device_volume(volume.audio_device, control, updated_channel);
}

std::vector<audio_device_unique_volume_set> adjust_volume(const args& args, search_result& result)
{
    if (args.disable_volume_control)
    {
        return {};
    }

    std::vector<audio_device_unique_volume_set> visitors_vect = generate_unique_volume_set(args, result);

    for (auto& visitor : visitors_vect)
    {
        adjust_volume(args, visitor.volume, visitor.control, visitor.channel, visitor.volume_set);
       
        if (args.test_volume_control)
        {
            audio_device_channel new_channel;
            try_get_audio_device_channel(visitor.volume.audio_device, visitor.control.name, visitor.channel.id, visitor.channel.type, new_channel);

            double percentage_error_double = ((new_channel.volume_percent - visitor.volume_set.volume) / (visitor.volume_set.volume * 1.0)) * 100.0;
            int percentage_error = (int)std::rint(percentage_error_double);
            visitor.volume_control_error_percent = percentage_error;
            visitor.volume_control_error = std::abs(new_channel.volume_percent - visitor.volume_set.volume);
        }
    }

    update_devices_volume(result);

    return visitors_vect;
}

bool test_volume_control(const args& args, const search_result& result)
{
    if (!args.test_volume_control)
    {
        return true;
    }

    std::vector<audio_device_unique_volume_set> audio_set_result = generate_unique_volume_set(args, result);

    for (auto& audio_set : audio_set_result)
    {                            
        audio_device_channel channel;
        try_get_audio_device_channel(audio_set.volume.audio_device, audio_set.control.name, audio_set.channel.id, audio_set.channel.type, channel);

        if (channel.volume_percent == audio_set.volume_set.volume)
        {
            continue;
        }

        if (audio_set.volume_set.volume_max_error == 0 && channel.volume_percent != audio_set.volume_set.volume)
        {
            return false;
        }

        if (!(channel.volume_percent <= (audio_set.volume_set.volume + audio_set.volume_set.volume_max_error) &&
            channel.volume_percent >= std::abs(audio_set.volume_set.volume - audio_set.volume_set.volume_max_error)))
        {
            return false;
        }
    }

    return true;
}

void print_adjust_volume_results(const args& args, const std::vector<audio_device_unique_volume_set>& audio_set_result)
{
    if (audio_set_result.size() == 0)
    {
        return;
    }

    if (!args.verbose || args.use_json || args.no_stdout)
    {
        return;
    }

    print(!args.disable_colors, fmt::emphasis::bold, "Volume control:\n\n");

    for (auto& audio_set : audio_set_result)
    {
        audio_device_channel updated_channel;   
        if (try_get_audio_device_channel(audio_set.volume.audio_device, audio_set.control.name, audio_set.channel.id, audio_set.channel.type, updated_channel))
        {
            double percentage_error_double = ((updated_channel.volume_percent - audio_set.volume_set.volume) / (audio_set.volume_set.volume * 1.0)) * 100.0;
            int percentage_error = (int)std::rint(percentage_error_double);
            std::string percentage_error_str = std::to_string(percentage_error);
            if (percentage_error > 0)
            {
                percentage_error_str = "+" + percentage_error_str;
            }
            print(!args.disable_colors, fmt::emphasis::bold, "    Volume set to \"{}%\" (actual: {}%) with {}% error on device \"{}\" for control name \"{}\" and {} channel \"{}\"\n", audio_set.volume_set.volume, updated_channel.volume_percent, percentage_error_str, audio_set.volume.audio_device.hw_id, audio_set.control.name, to_string(audio_set.channel.type), audio_set.channel.name);
        }
    }

    printf("\n");
}

void update_devices_volume(search_result& result)
{
    for (auto& d : result.devices)
    {
        try_get_audio_device_volume(d.first.audio_device, d.first);
    }
}

std::vector<audio_device_volume_probe> probe_volume_control(const args& args, const search_result& result)
{
    std::vector<audio_device_volume_probe> probe_result;

    if (!args.probe_volume_control)
    {
        return {};
    }

    for (const auto& device : result.devices)
    {
        for (const auto& control : device.first.controls)
        {
            for (auto channel : control.channels)
            {
                int initial_volume_value = channel.volume_percent;

                for (int i = 0; i <= 100; i++)
                {
                    channel.volume_percent = i;

                    try_set_audio_device_volume(device.first.audio_device, control, channel);
                    try_get_audio_device_channel(device.first.audio_device, control.name, channel.id, channel.type, channel);

                    audio_device_volume_probe probe;
                    probe.set_volume = i;
                    probe.retrieved_volume = channel.volume_percent;
                    probe.unique_channel_id = create_unique_channel_id(device.first.audio_device, control, channel);

                    probe_result.push_back(probe);
                }

                channel.volume_percent = initial_volume_value;
                try_set_audio_device_volume(device.first.audio_device, control, channel);
            }
        }
    }

    return probe_result;
}

void print(const args& args, const search_result& result, bool volume_control_return_value, const std::vector<audio_device_unique_volume_set>& audio_set_result)
{
    std::string config_file = std::filesystem::absolute(args.config_file).string();

    if (!args.no_stdout && !args.use_json && args.verbose && !args.ignore_config)
    {
        print(!args.disable_colors, fmt::emphasis::bold, "Using configuration file: ");
        print(!args.disable_colors, fg(fmt::color::gray), "{}\n", config_file);
    }

    std::string json_output = to_json(args, result, audio_set_result, volume_control_return_value);

    if (args.use_json && !args.no_stdout)
    {
        printf("%s\n", json_output.c_str());
    }
    else
    {
        print_stdout(args, result);
    }

    print_adjust_volume_results(args, audio_set_result);

    print_to_file(args, json_output);
}

int process_devices(const args& args)
{
    search_result result = search(args);

    std::vector<audio_device_volume_probe> probe_results = probe_volume_control(args, result);

    auto adjust_volume_results = adjust_volume(args, result);    

    bool volume_test_return_value = test_volume_control(args, result);

    int return_value = volume_test_return_value ? 0 : 1;

    print(args, result, volume_test_return_value, adjust_volume_results);

    bool generate_direwolf_result = generate_direwolf_output_file(args, result);

    if (volume_test_return_value)
    {
        if (result.devices.size() == 0 && result.ports.size() == 0)
        {
           return_value = 1;
        }
    }

    if (!generate_direwolf_result)
    {
        return_value = 1;
    }

    return return_value;
}

bool generate_direwolf_output_file(const args& args, const search_result& result)
{
    if (args.direwolf_output_file.empty())
    {
        return true;
    }

    std::string file_name = std::filesystem::absolute(args.direwolf_output_file).string();

    if (result.devices.size() != 1)
    {
        return false;
    }

    if (std::filesystem::exists(file_name))
    {
        std::filesystem::remove(file_name);
    }

    const auto& audio_device = result.devices[0];

    std::string lines;
    lines += "# AUTO-GENERATED BY find_devices, DO NOT CHANGE\n";
    lines += fmt::format("ADEVICE {}\n", audio_device.first.audio_device.plughw_id);
    if (result.ports.size() == 1)
    {
        lines += fmt::format("PTT {} RTS\n", result.ports[0].first.name);
    }
    lines += "ACHANNELS 1\n";
    lines += "CHANNEL 0\n";
    lines += "DTMF\n";
    lines += "MODEM 1200\n";
    if (!args.direwolf_callsign.empty())
    {
        lines += fmt::format("MYCALL {}\n", args.direwolf_callsign);
    }
    else
    {
        lines += fmt::format("MYCALL N0CALL\n");
    }
    if (args.direwolf_agwport != -1)
    {
        lines += fmt::format("AGWPORT {}\n", args.direwolf_agwport);
    }
    if (args.direwolf_kissport != -1)
    {
        lines += fmt::format("KISSPORT {}", args.direwolf_kissport);
    }

    write_line_to_file(file_name, lines);

    if (args.verbose && !args.use_json && !args.no_stdout)
    {
        print(!args.disable_colors, fmt::emphasis::bold, "Created Direwolf configuration file: ");
        print(!args.disable_colors, fg(fmt::color::red), "{}\n\n", file_name);
    }

    return true;
}

void print_version()
{
    printf(
        "find_devices - audio device and serial ports finding utility\n"
        "version "
        STRINGIFY(FIND_DEVICES_VERSION)
#ifdef GIT_HASH
        " "
        STRINGIFY(GIT_HASH)
#endif        
        "\n"
        "(C) 2023 Ion Todirel\n"
        "\n");
}