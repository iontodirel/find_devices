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

    bool try_find_new_filename(std::filesystem::path file_name, std::string& new_filename)
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
// DATA STRUCTURES                                                  //
//                                                                  //
//                                                                  //
// **************************************************************** //

struct audio_device_filter;
struct serial_port_filter;
enum class search_mode;
enum class included_devices;
struct args;
struct search_result;

struct audio_device_filter
{
    std::string name_filter = "";
    std::string desc_filter = "";
    bool playback_only = false; // use audio_device_type?
    bool capture_only = false; // use audio_device_type?
    bool playback_or_capture = false; // use audio_device_type?
    bool playback_and_capture = false; // use audio_device_type?
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
    bool verbose = true;
    bool help = false;
    bool use_json = false; 
    std::string output_file;
    bool disable_write_file = false;
    bool list_properties = false;
    int expected_count = -1;
    enum search_mode search_mode = search_mode::independent;
    enum included_devices included_devices = included_devices::all;
    std::string config_file = "config.json";
    bool ignore_config = false;
    bool disable_colors = false;
    bool no_stdout = false;
    bool command_line_has_errors = false;
    bool show_version = false;
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
    std::string device_name_filter = to_lower(m.name_filter);
    std::string device_desc = to_lower(d.description);
    std::string device_desc_filter = to_lower(m.desc_filter);

    if (!device_name_filter.empty() && !device_desc_filter.empty())
    {
        if (device_name.find(device_name_filter) == std::string::npos || device_desc.find(device_desc_filter) == std::string::npos)
        {
            return false;
        }
    }
    else
    {
        if ((!device_name_filter.empty() && device_name.find(device_name_filter) == std::string::npos) ||
            (!device_desc_filter.empty() && device_desc.find(device_desc_filter) == std::string::npos))
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

bool match_device(const device_description& p, const audio_device_filter& m)
{
    if (m.bus != -1 && m.bus != p.bus_number)
        return false;
    if (m.device != -1 && m.device != p.device_number)
        return false;
    if (m.topology != -1 && m.topology != p.topology_depth)
        return false;
    if (!m.path.empty() && m.path != p.path)
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
    if (!m.path.empty() && m.path != p.path)
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
search_mode parse_search_mode(const std::string& mode);
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
    if (args.search_mode == search_mode::independent)
    {
        result.devices = filter_audio_devices(args, get_audio_devices());
        result.ports = filter_serial_ports(args, get_serial_ports());
    }
    else if (args.search_mode == search_mode::port_siblings)
    {
        result.ports = filter_serial_ports(args, get_serial_ports());
        result.devices = filter_audio_devices(args, get_sibling_audio_devices(result.ports));        
    }
    else if (args.search_mode == search_mode::audio_siblings)
    {
        result.devices = filter_audio_devices(args, get_audio_devices());
        result.ports = filter_serial_ports(args, get_sibling_serial_ports(result.devices));
    }
    return result;
}

search_mode parse_search_mode(const std::string& mode)
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

// **************************************************************** //
//                                                                  //
// COMMAND LINE                                                     //
//                                                                  //
// **************************************************************** //

int print_usage();
bool has_command_line_property(const args& args, const std::string& arg);
bool config_file_present(const args& args);
args parse_command_line(int argc, char* argv[]);
void parse_audio_device_type(const std::string& typeStr, args& args);
included_devices parse_included_devices(const std::string& mode);

bool try_parse_command_line(int argc, char* argv[], args& args)
{
    cxxopts::Options options("find_devices", "Audio device and serial ports finding utility");
    
    options
        .allow_unrecognised_options()
        .add_options()
        ("audio.name", "", cxxopts::value<std::string>())        
        ("audio.desc", "", cxxopts::value<std::string>())
        ("audio.type", "", cxxopts::value<std::string>()->default_value("playback|capture"))
        ("audio.bus", "", cxxopts::value<int>())
        ("audio.device", "", cxxopts::value<int>())
        ("audio.path", "", cxxopts::value<std::string>())
        ("audio.topology", "", cxxopts::value<int>())
        ("port.name", "", cxxopts::value<std::string>())
        ("port.desc", "", cxxopts::value<std::string>())
        ("port.bus", "", cxxopts::value<int>())
        ("port.device", "", cxxopts::value<int>())
        ("port.topology", "", cxxopts::value<int>())
        ("port.path", "", cxxopts::value<std::string>())
        ("port.serial", "", cxxopts::value<std::string>())
        ("s,search-mode", "", cxxopts::value<std::string>()->default_value("independent"))
        ("p,list-properties", "")
        ("e,expected-count", "", cxxopts::value<int>()->default_value("1"))
        ("ignore-config", "")
        ("c,config-file", "", cxxopts::value<std::string>())
        ("o,output-file", "", cxxopts::value<std::string>())
        ("i,included-devices", "", cxxopts::value<std::string>()->default_value("all"))
        ("disable-colors", "")
        ("j,use-json", "")
        ("disable-file-write", "")
        ("v,version", "")
        ("verbose", "", cxxopts::value<bool>()->default_value("true"))
        ("no-verbose", "")
        ("no-stdout", "")
        ("h,help", "");

    cxxopts::ParseResult result;
    
    try
    {
        result = options.parse(argc, argv);
    }
    catch (const cxxopts::exceptions::incorrect_argument_type& e)
    {
        printf("Error parsing command line: %s\n\n", e.what());
        args.command_line_has_errors = true;
        print_usage();        
        return false;
    }
    catch (const std::exception& e)
    {
        printf("Error parsing command line.");
        args.command_line_has_errors = true;
        print_usage();
        return false;
    }    

    std::vector<std::string> unmatched = result.unmatched();
    if (unmatched.size() > 0)
    {
        fmt::print(fg(fmt::color::red), "Unknown command line argument: \"{}\"\n\n", unmatched[0]);
        args.command_line_has_errors = true;
        print_usage();
        return false;
    }

    if (result.count("help") > 0)
    {
        args.help = true;
        return true;
    }

    if (result.count("version") > 0)
    {
        args.show_version = true;
        printf(
            "find_devices - audio device and serial ports finding utility\n"        
            #ifdef GIT_HASH
            "version "
            STRINGIFY(GIT_HASH)
            #endif        
            "\n"
            "(C) 2023 Ion Todirel\n"
            "\n");
        return 0;
    }

    args.disable_colors = result["disable-colors"].as<bool>(); // invert
    args.disable_write_file = result["disable-file-write"].as<bool>();
    args.ignore_config = result["ignore-config"].as<bool>();
    args.list_properties = result["list-properties"].as<bool>();
    args.use_json = result["use-json"].as<bool>();
    args.search_mode = parse_search_mode(result["search-mode"].as<std::string>());
    args.included_devices = parse_included_devices(result["included-devices"].as<std::string>());
    args.verbose = result["verbose"].as<bool>();

    if (result.count("no-stdout") > 0)
    {
        args.no_stdout = true;
    }

    if (result.count("no-verbose") > 0)
    {
        args.verbose = false;
    }

    for (const auto& arg : result.arguments())
    {
        args.command_line_args.insert({arg.key(), arg.value()});
    }

    if (result.count("config-file") > 0)
    {
        args.config_file = result["config-file"].as<std::string>();
    }

    if (result.count("output-file") > 0)
    {
        args.output_file = result["output-file"].as<std::string>();
    }

    if (result.count("expected-count") > 0)
    {
        args.expected_count = result["expected-count"].as<int>();
    }

    if (result.count("audio.desc") > 0)
    {
        args.audio_filter.desc_filter = result["audio.desc"].as<std::string>();
    }

    if (result.count("audio.name") > 0)
    {
        args.audio_filter.name_filter = result["audio.name"].as<std::string>();
    }

    if (result.count("audio.type") > 0)
    {
        parse_audio_device_type(result["audio.type"].as<std::string>(), args);// should just use the enum for the data structure
    }

    if (result.count("audio.bus") > 0)
    {
        args.audio_filter.bus = result["audio.bus"].as<int>();
    }

    if (result.count("audio.device") > 0)
    {
        args.audio_filter.device = result["audio.device"].as<int>();
    }

    if (result.count("audio.topology") > 0)
    {
        args.audio_filter.topology = result["audio.topology"].as<int>();
    }

    if (result.count("audio.path") > 0)
    {
        args.audio_filter.path = result["audio.path"].as<std::string>();
    }

    if (result.count("port.name") > 0)
    {
        args.port_filter.name_filter = result["port.name"].as<std::string>();
    }

    if (result.count("port.desc") > 0)
    {
        args.port_filter.description_filter = result["port.desc"].as<std::string>();
    }

    if (result.count("port.bus") > 0)
    {
        try_parse_number(result["port.bus"].as<std::string>(), args.port_filter.bus);
    }
    
    if (result.count("port.device") > 0)
    {
        try_parse_number(result["port.device"].as<std::string>(), args.port_filter.device);
    }
    
    if (result.count("port.topology") > 0)
    {
        try_parse_number(result["port.topology"].as<std::string>(), args.port_filter.topology);
    }

    if (result.count("port.path") > 0)
    {
        args.port_filter.path = result["port.path"].as<std::string>();
    }

    if (result.count("port.serial") > 0)
    {
        args.port_filter.device_serial_number = result["port.serial"].as<std::string>();
    }

    return true;
}

bool config_file_present(const args& args)
{
    return std::filesystem::exists(args.config_file);
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

included_devices parse_included_devices(const std::string& mode)
{
    if (mode == "audio")
    {
        return included_devices::audio;
    }
    else if (mode == "ports")
    {
        return included_devices::ports;
    }
    else if (mode == "all")
    {
        return included_devices::all;
    }
    return included_devices::unknown;
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

    if (!args.command_line_args.contains("search-mode"))
        args.search_mode = parse_search_mode(j.value("search_mode", ""));
    if (!args.command_line_args.contains("expected-count"))
        try_parse_number(j.value("expected_count", ""), args.expected_count);
    if (!args.command_line_args.contains("use-json"))
        try_parse_bool(j.value("use_json", ""), args.use_json);
    if (!args.command_line_args.contains("list-properties"))
        try_parse_bool(j.value("list_properties", ""), args.list_properties);
    if (j.contains("output_file"))
    {
        if (!args.command_line_args.contains("output-file"))
        {
            args.disable_write_file = false;
            args.output_file = j["output_file"];
            args.output_file = get_full_path(args.output_file);
        }
    }
    if (!args.command_line_args.contains("included-devices"))
        args.included_devices = parse_included_devices(j.value("included_devices", ""));
    if (j.contains("search_criteria"))
    {
        nlohmann::json search_criteria = j["search_criteria"];
        if (search_criteria.contains("audio"))
        {
            nlohmann::json audio_match = search_criteria["audio"];
            if (!args.command_line_args.contains("audio.name"))
                args.audio_filter.name_filter = audio_match.value("name", "");
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
    args args;

    if (!try_parse_command_line(argc, argv, args))
    {
        return 1;
    }

    if (args.help)
    {
        print_usage();
        return 1;
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
        "    --audio.name <name>            search filter: partial or complete name of the audio device\n"
        "    --audio.desc <description>     search filter: partial or complete description of the audio device\n"
        "    --audio.type <type>            search filter: types of audio devices to find: playback, capture, playback|capture, playback&capture:\n"
        "                                       playback - playback only\n"
        "                                       capture - capture only\n"
        "                                       \"playback|capture\" - playback or capture\n"
        "                                       \"playback&capture\" - playback and capture\n"
        "                                       all\n"
        "    --audio.bus <number>           search filter: audio device bus number\n"
        "    --audio.device <number>        search filter: audio device number\n"
        "    --audio.path <path>            search filter: audio device hardware system path\n"
        "    --audio.topology <number>      search filter: the depth of the audio device topology, in the device tree\n"
        "    --port.name <serial>           search filter: partial or complete name of the serial port\n"
        "    --port.desc <serial>           search filter: partial or complete description of the serial port\n"
        "    --port.bus <number>            search filter: serial port bus number\n"
        "    --port.device <number>         search filter: partial or complete serial port device serial number\n"
        "    --port.topology <number>       search filter: partial or complete serial port device serial number\n"
        "    --port.path <path>             search filter: partial or complete serial port device serial number\n"
        "    --port.serial <serial>         search filter: partial or complete serial port device serial number\n"        
        "    -v, --verbose                  enable detailed printing to stdout\n"
        "    --no-verbose                   disable detailed printing to stdout\n"
        "    --no-stdout                    don't print to stdout\n"
        "    -h, --help                     print help\n"
        "    -v, --version                  prints the version of this program\n"
        "    -p, --list-properties          print detailed properties for each device and serial port\n"
        "    -i, --included-devices <type>  type of devices to include in searches and in stdout or JSON:\n"
        "                                       audio - include audio devices\n"
        "                                       ports - include serial ports\n"
        "                                       all - include audio devices and serial ports\n"
        "    -s, --search-mode <mode>       how to conduct the search: \n"
        "                                       independent - look for audio devices and serial ports independently\n"
        "                                       audio-siblings - look for audio devices and find their sibling serial ports\n"
        "                                       port-siblings - look for serial ports and find their sibling audio devices\n"
        "    -e, --expected <count>         how many results to expect from a search\n"
        "                                   devices of each type count as one, one serial port and one audio device count as one\n"
        "                                   default value is one, if the result count does not match the count, return value will be 1\n"
        "    --output-file <file>           write results as JSON to a file\n"
        "    --json                         display JSON to stdout\n"
        "    --ignore-config                ignore the configuration file, if a configurtion file is available or specified\n"
        "    -c, --config-file <file>       use a configuration file to configure the program\n"
        "                                   settings specified as command line args override settings present in the config file\n"
        "                                   if not specified default config file name used is \"config.json\"\n"
        "    --disable-colors               do not print colors in stdout\n"
        "    --disable-file-write           disables writing a JSON file with the results of the search, which is the defaul\n"
        "    --test-data    <file>          fake the data as if it came from the system, for testing purposes\n"
        "\n"
        "Returns:\n"
        "    0 - success, audio devices or serial ports are found matching the search criteria\n"
        "    1 - if the command line arguments are incorrect, or if called with --help\n"
        "    1 - if no devices are found, or no devices are matching the search criteria\n"
        "    1 - if the number of devices found do not match the count specified by --expected\n"
        "\n"
        "Example:\n"
        "    find_devices --audio.name \"USB Audio\" --audio.desc \"Texas Instruments\" --no-verbose\n"
        "    find_devices --audio.desc \"C-Media Electronics Inc.\" -s audio-siblings -i all\n"
        "    find_devices --audio.desc \"C-Media\" --port.desc \"CP2102N\" -s port-siblings -i all \n"
        "    find_devices --audio.bus=2 --audio.device=48 -s audio-siblings -i audio \n"
        "    find_devices --audio.type \"playback&capture\"\n"
        "    find_devices -h\n"
        "    find_devices -j -o output.json\n"
        "    find_devices -c digirig_config.json\n"
        "\n"
        "Defaults:\n"
        "    --verbose\n"
        "    --audio.type all\n"
        "    --include-device-types all\n"
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
            if (!args.verbose )
                printf("%s\n", d.first.plughw_id.c_str());
            else
            {
                print(!args.disable_colors, fmt::emphasis::bold, "{:>4})", i);
                print(!args.disable_colors, fmt::emphasis::bold | fg(fmt::color::chartreuse), " {}", d.first.hw_id);
                fmt::print(": ");
                print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::cornflower_blue), "{}", d.first.name);
                fmt::print(" - ");
                print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::chocolate), "{}", d.first.description);
                fmt::println("");

                if (args.list_properties)
                {
                    fmt::println("");
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "hwid");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.hw_id);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "plughwid");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.plughw_id);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "name");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.name);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "description");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.description);
                    print(!args.disable_colors, fmt::emphasis::bold | fmt::emphasis::italic | fg(fmt::color::rosy_brown), "{:>20}: ", "stream name");
                    print(!args.disable_colors, fmt::emphasis::italic | fg(fmt::color::gray), "{}\n", d.first.stream_name);                
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
        return;
    
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

int list_devices(args& args)
{
    search_result result = search(args);

    std::string json_output = to_json(result);

    if (args.use_json && !args.no_stdout)
    {
        printf("%s\n", json_output.c_str());
    }
    else
    {
        print(args, result);
    }

    print_to_file(args, json_output);

    return 0;
}
