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
#include <memory>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <optional>
#include <tuple>
#include <iostream>
#include <fstream>
#include <set>

#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <fmt/color.h>

// **************************************************************** //
//                                                                  //
// UTILITIES                                                        //
//                                                                  //
// **************************************************************** //

#define STR(x) #x
#define STRINGIFY(x) STR(x)

namespace
{
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

    bool try_parse_bool(std::string s, bool& b)
    {
        if (s == "true")
            b = true;
        else if (s == "false")
            b = false;
        else
            return false;
        return true;
    }

    std::string get_full_path(std::string path)
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
}

// **************************************************************** //
//                                                                  //
//                                                                  //
// DATA STRUCTURES                                                  //
//                                                                  //
//                                                                  //
// **************************************************************** //

struct audio_device_filter;
struct serial_port_filter;
enum class search_mode;
enum class display_mode;
struct args;
struct search_result;

struct audio_device_filter
{
    std::string device_name_filter = "";
    std::string device_desc_filter = "";
    bool playback_only = false;
    bool capture_only = false;
    bool playback_or_capture = false;
    bool playback_and_capture = false;
    int bus = -1;
    int device = -1;
    int topology = -1;
    std::string path;
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

enum class display_mode
{
    not_set,
    audio,
    ports,
    audio_and_ports
};

struct args
{
    std::vector<std::pair<std::string, std::string>> command_line_args;
    audio_device_filter audio_filter;
    serial_port_filter port_filter;
    bool verbose = true;
    bool list = false;
    bool help = false;
    bool json_output = false;
    std::string file;
    bool print_to_file = false;
    bool list_properties = false;
    int expected_count = -1;
    search_mode search_strategy = search_mode::not_set;
    display_mode view_mode = display_mode::not_set;
    std::string config_file;
    bool ignore_config = false;
    bool pretty_print = true;
};

struct search_result
{
    std::vector<std::pair<audio_device_info, device_description>> devices;
    std::vector<std::pair<serial_port, device_description>> ports;
};

// **************************************************************** //
//                                                                  //
// AUDIO DEVICES                                                    //
//                                                                  //
// **************************************************************** //

std::vector<audio_device_info> get_audio_devices(const audio_device_filter& m);
bool match_audio_device(const audio_device_info& d, const audio_device_filter& m);

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
    std::string device_name_filter = to_lower(m.device_name_filter);
    std::string device_desc = to_lower(d.description);
    std::string device_desc_filter = to_lower(m.device_desc_filter);

    if (device_name_filter.size() > 0 && device_desc_filter.size() > 0)
    {
        if (device_name.find(device_name_filter) == std::string::npos || device_desc.find(device_desc_filter) == std::string::npos)
        {
            return false;
        }
    }
    else
    {
        if ((device_name_filter.size() > 0 && device_name.find(device_name_filter) == std::string::npos) ||
            (device_desc_filter.size() > 0 && device_desc.find(device_desc_filter) == std::string::npos))
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

// **************************************************************** //
//                                                                  //
// SERIAL PORTS                                                     //
//                                                                  //
// **************************************************************** //

std::vector<serial_port> get_serial_ports(const serial_port_filter& m);
bool match_port(const serial_port& p, const serial_port_filter& m);
bool match_device(const device_description& p, const audio_device_filter& m);
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

    if (port_name_filter.size() > 0 && port_name.find(port_name_filter) == std::string::npos)
        return false;
    if (port_desc_filter.size() > 0 && port_desc.find(port_desc_filter) == std::string::npos)
        return false;
    if (port_mfd_filter.size() > 0 && port_mfd.find(port_mfd_filter) == std::string::npos)
        return false;
    if (port_serial_filter.size() > 0 && port_serial.find(port_serial_filter) == std::string::npos)
        return false;

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
    if (m.path.size() > 0 && m.path != p.path)
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
    if (m.path.size() > 0 && m.path != p.path)
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
search_result search(const args& args);
search_mode parse_from_string(const std::string& mode);
bool has_audio_device_description_filter(const args& args);
bool has_serial_port_description_filter(const args& args);

std::string to_json(const search_result& result);

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

search_result search(const args& args)
{
    search_result result;
    if (args.search_strategy == search_mode::independent)
    {
        result.devices = filter_audio_devices(args, get_audio_devices());
        result.ports = filter_serial_ports(args, get_serial_ports());
    }
    else if (args.search_strategy == search_mode::port_siblings)
    {
        result.ports = filter_serial_ports(args, get_serial_ports());
        result.devices = filter_audio_devices(args, get_sibling_audio_devices(result.ports));        
    }
    else if (args.search_strategy == search_mode::audio_siblings)
    {
        result.devices = filter_audio_devices(args, get_audio_devices());
        result.ports = filter_serial_ports(args, get_sibling_serial_ports(result.devices));
    }
    return result;
}

search_mode parse_from_string(const std::string& mode)
{
    if (mode == "independent")
    {
        return search_mode::independent;
    }
    else if (mode == "audio-siblings")
    {
        return search_mode::audio_siblings;
    }
    else if (mode == "port-siblings")
    {
        return search_mode::port_siblings;
    }
    return search_mode::not_set;
}

std::string to_json(const search_result& result)
{
    std::string s;
    s += "{\n";
    s += "    \"audio_devices\": [\n";
    size_t i = 0;
    for (const auto& d : result.devices)
    {
        s += "        {\n";
        s += to_json(d.first, false, 2);
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

    s += "    ]\n";
    s += "}";

    return s;
}

bool has_audio_device_description_filter(const args& args)
{
    return (args.audio_filter.bus != -1 ||
        args.audio_filter.device != -1 || 
        args.audio_filter.path.size() > 0 ||
        args.audio_filter.topology != -1);
}

bool has_serial_port_description_filter(const args& args)
{
    return (args.port_filter.bus != -1 ||
        args.port_filter.device != -1 || 
        args.port_filter.path.size() > 0 ||
        args.port_filter.topology != -1);
}

// **************************************************************** //
//                                                                  //
// COMMAND LINE                                                     //
//                                                                  //
// **************************************************************** //

args parse_command_line(int argc, char* argv[]);
void parse_audio_device_type(const std::string& typeStr, args& args);
display_mode parse_print_mode(const std::string& mode);

args parse_command_line(int argc, char* argv[])
{
    args args;

    bool audio_group = false;
    bool port_group = false;

    for (int i = 0; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg.find("--name") != std::string::npos && (i + 1) < argc)
        {
            if (audio_group || (!audio_group && !port_group))
            {
                args.audio_filter.device_name_filter = argv[i + 1];
            }
            else if (port_group)
            {
                args.port_filter.name_filter = argv[i + 1];
            }
            args.command_line_args.push_back(std::make_pair(arg, argv[i + 1]));
            i++;
            continue;
        }
        else if (arg.find("--desc") != std::string::npos && (i + 1) < argc)
        {
            if (audio_group || (!audio_group && !port_group))
            {
                args.audio_filter.device_desc_filter = argv[i + 1];
            }
            else if (port_group)
            {
                args.port_filter.description_filter = argv[i + 1];
            }
            args.command_line_args.push_back(std::make_pair(arg, argv[i + 1]));
            i++;
            continue;
        }
         if (arg.find("--serial") != std::string::npos && (i + 1) < argc)
        {
            args.port_filter.device_serial_number = argv[i + 1];
            args.command_line_args.push_back(std::make_pair(arg, argv[i + 1]));
            i++;
            continue;
        }
        else if (arg.find("--verbose") != std::string::npos || arg.find("-v") != std::string::npos)
        {
            args.verbose = true;
            args.command_line_args.push_back(std::make_pair(arg, ""));
        }
        else if (arg.find("--no-verbose") != std::string::npos)
        {
            args.verbose = false;
            args.command_line_args.push_back(std::make_pair(arg, ""));
        }
        else if (arg.find("--help") != std::string::npos || arg.find("-h") != std::string::npos)
        {
            args.help = true;
            args.command_line_args.push_back(std::make_pair(arg, ""));
        }
        else if ((arg == "--type" || arg == "-t") && (i + 1) < argc)
        {
            args.audio_filter.capture_only = false;
            args.audio_filter.playback_only = false;
            args.audio_filter.playback_or_capture = false;
            args.audio_filter.playback_and_capture = false;
            parse_audio_device_type(argv[i + 1], args);
            args.command_line_args.push_back(std::make_pair(arg, argv[i + 1]));
            i++;
            continue;
        }
        else if (arg == "--list" || arg == "-l")
        {
            args.list = true;
            args.command_line_args.push_back(std::make_pair(arg, ""));
        }
        else if (arg == "--properties" || arg == "-p")
        {
            args.list_properties = true;
            args.command_line_args.push_back(std::make_pair(arg, ""));
        }
        else if (arg == "--print" && (i + 1) < argc)
        {
            args.view_mode = parse_print_mode(argv[i + 1]);
            args.command_line_args.push_back(std::make_pair(arg, argv[i + 1]));
            i++;
            continue;
        }
        else if (arg == "--search-mode" && (i + 1) < argc)
        {
            args.search_strategy = parse_from_string(argv[i + 1]);
            args.command_line_args.push_back(std::make_pair(arg, argv[i + 1]));
            i++;
            continue;
        }
        else if ((arg == "-c" || arg == "--expected") && (i + 1) < argc)
        {
            try_parse_number(argv[i + 1], args.expected_count);
            args.command_line_args.push_back(std::make_pair(arg, argv[i + 1]));
            i++;
            continue;
        }
        else if (arg == "--json")
        {
            args.json_output = true;
            args.command_line_args.push_back(std::make_pair(arg, ""));
        }
        else if (arg == "--file" && (i + 1) < argc)
        {
            args.file = argv[i + 1];
            args.file = get_full_path(args.file);
            args.print_to_file = true;
            args.command_line_args.push_back(std::make_pair(arg, argv[i + 1]));
            i++;
            continue;
        }
        else if (arg == "--audio-begin")
        {
            audio_group = true;
            args.command_line_args.push_back(std::make_pair(arg, ""));
        }
        else if (arg == "--audio-end")
        {
            audio_group = false;
            args.command_line_args.push_back(std::make_pair(arg, ""));
        }
        else if (arg == "--port-begin")
        {
            port_group = true;
            args.command_line_args.push_back(std::make_pair(arg, ""));
        }
        else if (arg == "--port-end")
        {
            port_group = false;
            args.command_line_args.push_back(std::make_pair(arg, ""));
        }
        else if (arg == "--ignore-config")
        {
            args.ignore_config = true;
            args.command_line_args.push_back(std::make_pair(arg, ""));
        }
        else if (arg == "--disable-colors")
        {
            args.pretty_print = false;
            args.command_line_args.push_back(std::make_pair(arg, ""));
        }
        else if (arg == "--config" && (i + 1) < argc)
        {
            args.config_file = argv[i + 1];
            args.command_line_args.push_back(std::make_pair(arg, argv[i + 1]));
            i++;
            continue;
        }
    }

    if (args.command_line_args.size() == 0)
    {
        if ((!std::filesystem::exists(args.config_file) && !std::filesystem::exists("config.json")) || args.ignore_config)
        {            
            args.list = true;
            args.verbose = true;
            args.audio_filter.playback_or_capture = true;
            args.view_mode = display_mode::audio;
        }
    }

    return args;
}

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

display_mode parse_print_mode(const std::string& mode)
{
    if (mode == "audio")
    {
        return display_mode::audio;
    }
    else if (mode == "ports")
    {
        return display_mode::ports;
    }
    else if (mode == "audio, ports" || mode == "audio,ports" ||
    mode == "ports, audio" || mode == "ports,audio")
    {
        return display_mode::audio_and_ports;
    }
    return display_mode::not_set;
}

// **************************************************************** //
//                                                                  //
// SETTINGS                                                         //
//                                                                  //
// **************************************************************** //

void read_settings(args& args);

void read_settings(args& args)
{
    if (args.ignore_config)
        return;
    
    std::string config_file = args.config_file;

    if (args.config_file.size() == 0)
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
        i >> j;
    }
    catch (nlohmann::json::parse_error&)
    {
        return;
    }
    catch (nlohmann::json::type_error&)
    {
        return;
    }

    if (args.search_strategy == search_mode::not_set)
        args.search_strategy = parse_from_string(j.value("search_mode", ""));
    if (args.expected_count == -1)
        try_parse_number(j.value("expected_count", ""), args.expected_count);
    if (!args.json_output)
        try_parse_bool(j.value("use_json", ""), args.json_output);
    if (!args.list_properties)
        try_parse_bool(j.value("list_properties", ""), args.list_properties);
    if (j.contains("output_file"))
    {
        if (args.file.size() == 0)
        {
            args.print_to_file = true;
            args.file = j["output_file"];
            args.file = get_full_path(args.file);
        }
    }
    if (args.view_mode == display_mode::not_set)
        args.view_mode = parse_print_mode(j.value("display_mode", ""));
    if (j.contains("search_criteria"))
    {
        nlohmann::json search_criteria = j["search_criteria"];
        if (search_criteria.contains("audio"))
        {
            nlohmann::json audio_match = search_criteria["audio"];
            if (args.audio_filter.device_name_filter.size() == 0)
                args.audio_filter.device_name_filter = audio_match.value("name", "");
            if (args.audio_filter.device_desc_filter.size() == 0)
                args.audio_filter.device_desc_filter = audio_match.value("desc", "");
            if (!args.audio_filter.playback_or_capture && 
                !args.audio_filter.playback_only && 
                !args.audio_filter.capture_only && 
                !args.audio_filter.playback_and_capture)
                parse_audio_device_type(audio_match.value("type", ""), args);
            if (args.audio_filter.bus == -1)
                try_parse_number(audio_match.value("bus", ""), args.audio_filter.bus);
            if (args.audio_filter.device == -1)
                try_parse_number(audio_match.value("device", ""), args.audio_filter.device);
            if (args.audio_filter.topology == -1)
                try_parse_number(audio_match.value("topology_depth", ""), args.audio_filter.topology);
            if (args.audio_filter.path.size() == 0)
                args.audio_filter.path = audio_match.value("path", "");
        }
        if (search_criteria.contains("port"))
        {
            nlohmann::json port_match = search_criteria["port"];
            if (args.port_filter.name_filter.size() == 0)
                args.port_filter.name_filter = port_match.value("name", "");
            if (args.port_filter.description_filter.size() == 0)
                args.port_filter.description_filter = port_match.value("desc", "");     
            if (args.port_filter.bus == -1)
                try_parse_number(port_match.value("bus", ""), args.port_filter.bus);
            if (args.port_filter.device == -1)
                try_parse_number(port_match.value("device", ""), args.port_filter.device);
            if (args.port_filter.topology == -1)
                try_parse_number(port_match.value("topology_depth", ""), args.port_filter.topology);
            if (args.port_filter.path.size() == 0)
                args.port_filter.path = port_match.value("path", "");
            if (args.port_filter.device_serial_number.size() == 0)
                args.port_filter.device_serial_number = port_match.value("serial", "");
        }
    }
}

// **************************************************************** //
//                                                                  //
// MAIN                                                             //
//                                                                  //
// **************************************************************** //

int main(int argc, char* argv[]);
int print_usage();
void print(args& args, search_result& result);
int list_devices(args& args);

int main(int argc, char* argv[])
{
    args args = parse_command_line(argc, argv);

    if (args.help)
    {
        return print_usage();
    }

    read_settings(args);

    return list_devices(args);
}

int print_usage()
{
    std::string usage =
        "find_devices - audio device and serial ports finding utility\n"        
        #ifdef GIT_HASH
        "version "
        STRINGIFY(GIT_HASH)
        #endif        
        "\n"
        "(C) 2023 Ion Todirel\n"
        "\n"
        "Usage:\n"
        "    find_devices [OPTION]... \n"
        "\n"
        "Options:\n"
        "    --audio-begin            beginning of an audio device search group\n"
        "    --audio-end              end of an audio device search group\n"
        "    --port-begin             beginning of an serial port search group\n"
        "    --port-end               end of an serial port search group\n"
        "    --name <name>            search criteria: partial or complete name of the audio device\n"
        "    --desc <description>     search criteria: partial or complete description of the audio device\n"
        "    -t, --type <type>        search criteria: types of audio devices to find: playback, capture, playback|capture, playback&capture:\n"
        "                             playback - playback only\n"
        "                             capture - capture only\n"
        "                             playback|capture - playback or capture\n"
        "                             playback&capture - playback and capture\n"
        "    --serial <serial>        search criteria: partial or complete deice serial number\n"
        "    --bus <number>           search criteria: device bus number, stable between restarts\n"
        "    --device <number>        search criteria: device number, stable between restarts\n"
        "    --path <path>            search criteria: device system path\n"
        "    --topology <n>           search criteria: the depth of the device topology\n"
        "    -v, --verbose            enable verbose stdout printing from this utility\n"
        "    --no-verbose             machine parsable output\n"
        "    -h, --help               print help\n"
        "    -l, --list               list devices, implicit\n"
        "    -p, --properties         print detailed properties of each device\n"
        "    --print <type>           only applies to stdout, type of information to print:\n"
        "                             audio - print audio devices\n"
        "                             ports - print serial ports\n"
        "                             \"audio, ports\" - print audio devices and serial ports\n"
        "    --search-mode <mode>     the mode in which to conduct the search: \n"
        "                             independent - look for audio devices and ports independently\n"
        "                             audio-siblings - look for audio devices and find their sibling serial ports\n"
        "                             port-siblings - look for serial ports and find their sibling audio devices\n"
        "    -c, --expected <count>   how many results to expect from a search\n"
        "                             devices of each type count as one, one serial port and one audio device count as one\n"
        "                             default value is one, if the result count does not match the count, return value will be 1\n"
        "    --file <file>            write results to a file, only applies to JSON output when used in conjunction with --json\n"
        "    --json                   display or write all information as JSON\n"
        "    --ignore-config          ignore the configuration JSON file if present\n"
        "    --config    <file>       use a configuration file, settings specified as command line args override the file config\n"
        "    --disable-colors         do not print colors in stdout\n"
        "\n"
        "Returns:\n"
        "    0 - success, devices are found, and they are matching the search criteria\n"
        "    1 - if the command line arguments are incorrect, or if called with --help\n"
        "    1 - if no devices are found, or no devices are matching the search criteria\n"
        "    1 - if the number of devices found do not match the count specified by --expected\n"
        "\n"
        "Example:\n"
        "    find_devices --name \"USB Audio\" --desc \"Texas Instruments\" --no-verbose\n"
        "    find_devices --desc \"C-Media Electronics Inc.\" --search-mode audio-siblings --print \"audio, ports\" \n"
        "    find_devices --audio-begin --desc \"C-Media\" --audio-end --port-begin --desc \"CP2102N\" --port-end --search-mode port-siblings --print \"audio, ports\" \n"
        "    find_devices --audio-begin --bus 2 --device 48 --audio-end --search-mode audio-siblings --print \"audio, ports\" \n"
        "    find_devices --list\n"
        "    find_devices --list --type \"playback|capture\"\n"
        "    find_devices --help\n"
        "    find_devices --list --json --file out.json\n"
        "    find_devices --config config.json\n"
        "\n"
        "Defaults:\n"
        "    --verbose\n"
        "    --type \"playback|capture\"\n"
        "    --print audio\n"
        "\n";
    printf("%s", usage.c_str());
    return 1;
}

template <typename... Args>
void print(bool enable_colors, const fmt::text_style& ts, const Args&... args)
{
    fmt::print(enable_colors ? ts : fmt::text_style(), args...);
}

void print(args& args, search_result& result)
{
    if (args.view_mode == display_mode::audio_and_ports || args.view_mode == display_mode::audio)
    {
        if (args.verbose)
        {
            print(args.pretty_print, fmt::emphasis::bold, "\nFound audio devices:\n\n");
        }

        // Current formatting settings support audio device count of up to 999
        // and up to 999 serial ports before breaking formatting

        size_t i = 1;
        for (const auto& d : result.devices)
        {
            if (!args.verbose)
                printf("%s\n", d.first.plughw_id.c_str());
            else
            {
                print(args.pretty_print, fmt::emphasis::bold, "{:>4})", i);
                print(args.pretty_print, fmt::emphasis::bold | fg(fmt::color::chartreuse), " {}", d.first.hw_id);
                fmt::print(": ");
                print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::cornflower_blue), "{}", d.first.name);
                fmt::print(" - ");
                print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::chocolate), "{}", d.first.description);
                fmt::println("");

                if (args.list_properties)
                {
                    fmt::println("");
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "hwid");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.hw_id);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "plughwid");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.plughw_id);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "name");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.name);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "description");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.description);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "stream name");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.stream_name);                
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "bus");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.bus_number);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "device");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.device_number);                
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "product");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.product);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "idProduct");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.id_product);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "idVendor");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.id_vendor);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "path");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.path);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "hardware path");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.hw_path);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "depth");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.second.topology_depth);
                    if (i < result.devices.size())
                        fmt::println("");
                }
            }
            i++;
        }
    }

    if (args.view_mode == display_mode::audio_and_ports || args.view_mode == display_mode::ports)
    {
        if (args.verbose)
        {
            print(args.pretty_print, fmt::emphasis::bold, "\nFound serial ports:\n\n");
        }

        size_t j = 1;
        for (const auto& p : result.ports)
        {
            if (!args.verbose)
                printf("%s\n", p.first.name.c_str());
            else
            {
                print(args.pretty_print, fmt::emphasis::bold, "{:>4})", j);
                print(args.pretty_print, fmt::emphasis::bold | fg(fmt::color::chartreuse), " {}", p.first.name);
                fmt::print(": ");
                print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::cornflower_blue), "{}", p.first.manufacturer);
                fmt::print(" - ");
                print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::chocolate), "{}", p.first.description);
                fmt::println("");
    
                if (args.list_properties)
                {
                    fmt::println("");
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "name");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.first.name);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "manufacturer");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.first.manufacturer);                
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "description");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.first.description);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "sn");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.first.device_serial_number);                
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "bus");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.bus_number);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "device");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.device_number);                
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "product");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.product);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "idProduct");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.id_product);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "idVendor");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.id_vendor);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "path");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.path);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "hardware path");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.hw_path);
                    print(args.pretty_print, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "depth");
                    print(args.pretty_print, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", p.second.topology_depth);
                    if (j < result.ports.size())
                        fmt::println("");
                }
            }
            j++;
        }
    }

    printf("\n");

    if (args.verbose && args.print_to_file)
    {
        print(args.pretty_print, fmt::emphasis::bold, "Wrote to file: ");
        print(args.pretty_print, fg(fmt::color::red), "{}\n\n", args.file);
    }
}

int list_devices(args& args)
{
    search_result result = search(args);

    std::string json_output = to_json(result);

    if (args.json_output)
    {
        printf("%s\n", json_output.c_str());
    }
    else
    {
        print(args, result);
    }

    if (args.print_to_file)
    {
        if (std::filesystem::exists(args.file))
            std::filesystem::remove(args.file);
        
        write_line_to_file(args.file, json_output);
    }

    return 0;
}
