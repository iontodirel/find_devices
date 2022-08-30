// main.cpp
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

#include <map>
#include <fstream>
#include <memory>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <optional>

struct resource_load_lazy_t
    {
    };

constexpr resource_load_lazy_t resource_load_lazy;

class resources
    {
    public:
        std::map<std::string, std::string> strings;

        void reset_load_next_position()
        {
            position = 0;
        }

        std::optional<std::pair<std::string, std::string>> load_next(const std::string& lang = "en-us")
        {
            std::string file_name = "strings." + lang + ".txt";

            if (!std::filesystem::exists(file_name))
            {
                return std::nullopt;
            }

            std::ifstream file(file_name);

            file.seekg(position);

            std::string line;
            std::string current_string_name;
            std::string current_string_value;
            int current_line_count = 0;
            bool current_line_begin = false;

            while (std::getline(file, line))
            {
                if (line.size() > 2)
                {
                    if (line[0] == '/' && line[1] == '/')
                    {
                        continue;
                    }
                    if (line[0] == '<' && line[line.size() - 1] == '>')
                    {
                        if (line[1] == '/')
                        {
                            //strings[current_string_name] = current_string_value;
                            
                            //current_line_count = 0;
                            current_line_begin = false;
                            
                            
                            //current_string_value = "";
                            break;
                        }
                        else if (line[1] != '<')
                        {
                            current_string_name = line.substr(1, line.size() - 2);
                            current_line_begin = true;
                        }
                        else if (line[1] == '<' && line[line.size() - 2] == '>')
                        {
                            current_string_value = line.substr(2, line.size() - 3);
                        }
                        continue;
                    }
                }
                if (!current_line_begin)
                    continue;
                current_string_value += (line.size() == 0) ? newline : line;
                if (current_line_count > 0)
                    current_string_value += newline;
                current_line_count++;
            }

            position = file.tellg();

            if (current_line_begin || current_line_count > 0)
            {
                return std::make_pair(current_string_name, current_string_value);
            }

            return std::nullopt;
            
        }

        const std::map<std::string, std::string>& load(const std::string& lang = "en-us")
        {
            strings.clear();
            
            reset_load_next_position();

            std::optional<std::pair<std::string, std::string>> current;
            while ((current = load_next(lang)))
            {
                strings[current.value().first] = current.value().second;
            }


          /*  std::string file_name = "strings." + lang + ".txt";

            if (!std::filesystem::exists(file_name))
                return strings;
            
            std::ifstream file(file_name);

            


            std::string line;
            std::string current_string_name;
            std::string current_string_value;
            int current_line_count = 0;
            bool current_line_begin = false;
            
            while (std::getline(file, line))
            {
                if (line.size() > 2)
                {
                    if (line[0] == '/' && line[1] == '/')
                    {
                        continue;
                    }
                    if (line[0] == '<' && line[line.size() - 1] == '>')
                    {
                        if (line[1] == '/')
                        {
                            strings[current_string_name] = current_string_value;
                            current_string_value = "";
                            current_line_count = 0;
                            current_line_begin = false;
                        }
                        else if (line[1] != '<')
                        {
                            current_string_name = line.substr(1, line.size() - 2);
                            current_line_begin = true;
                        }
                        else if (line[1] == '<' && line[line.size() - 2] == '>')
                        {
                            current_string_value = line.substr(2, line.size() - 3);
                        }
                        continue;
                    }
                }
                if (!current_line_begin)
                    continue;
                current_string_value += (line.size() == 0) ? "\n" : (line);
                if (current_line_count > 0)
                    current_string_value += "\n";
                current_line_count++;
            }*/
            return strings;
        }

        const std::string& string(const std::string& name)
        {
            return string(name, not_found);
        }

        std::string string(const std::string& name, resource_load_lazy_t, const std::string& lang = "en-us")
        {
            return string(name, not_found, resource_load_lazy, lang);
        }

        std::string string(const std::string& name, const std::string& default_value, resource_load_lazy_t, const std::string& lang = "en-us")
        {
            reset_load_next_position();

            std::optional<std::pair<std::string, std::string>> current;
            while ((current = load_next(lang)))
            {
                if (current.value().first == name)
                {
                    return current.value().second;
                }
            }

            return default_value;
        }

        const std::string& string(const std::string& name, const std::string& default_value)
        {
            if (strings.find(name) == strings.end())
                return default_value;
            return strings[name];
        }
    
        private:
            std::streampos position = 0;
            std::string newline = "\n";
            std::string not_found = "Resource not found in the string map.";
    };

resources res;

struct command_line_args
{
    std::map<std::string, std::string> args;
    audio_device_match match;
    bool verbose = true;
    bool list = false;
    bool help = false;
    bool json_output = false;
    std::string file;
    bool print_to_file = false;
    std::string lang = "en-us";
    bool lock = false;
};

command_line_args parse_command_line_args(int argc, char* argv[]);
int list_devices(const command_line_args& args);
int print_usage();
void append_line_to_file(const std::string& path, const std::string& line);

template<class F, class ...Args>
std::string format(F f, Args... args)
{
    int size_s = snprintf(nullptr, 0, f, args...) + 1;
    auto size = static_cast<size_t>(size_s);
    if (size > 0)
    {
        std::unique_ptr<char[]> buf(new char[size]);
        if (snprintf(buf.get(), size, f, args ...) > 0)
        {
            return std::string(buf.get(), buf.get() + size - 1);
        }
    }
    return "";
}

template<class F, class ...Args>
void print_stdout(F f, Args... args)
{
    printf(format(f, args ...).c_str());
}

void append_line_to_file(const std::string& path, const std::string& line)
{
    std::ofstream file;
    file.open(path, std::ios_base::app);
    file << line << "\n";
    file.close();
}

template<class F, class ...Args>
void print_out(const command_line_args& a, F f, Args... args)
{
    if (a.print_to_file)
    {
        append_line_to_file(a.file, format(f, args...));
    }
    else
    {
        printf(f, args...);
    }
}

command_line_args parse_command_line_args(int argc, char* argv[])
{
    command_line_args args;

    for (int i = 0; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg.find("--name") != std::string::npos && (i + 1) < argc)
        {
            args.match.device_name_filter = argv[i + 1];
            args.args["--name"] = argv[i + 1];
            i++;
            continue;
        }
        else if (arg.find("--desc") != std::string::npos && (i + 1) < argc)
        {
            args.match.device_desc_filter = argv[i + 1];
            args.args["--desc"] = argv[i + 1];
            i++;
            continue;
        }
        else if (arg.find("--verbose") != std::string::npos)
        {
            args.verbose = true;
            args.args["--verbose"] = "";
        }
        else if (arg.find("--no-verbose") != std::string::npos)
        {
            args.verbose = false;
            args.args["--no-verbose"] = "";
        }
        else if (arg.find("--help") != std::string::npos)
        {
            args.help = true;
            args.args["--help"] = "";
        }
        else if (arg.find("--type") != std::string::npos && (i + 1) < argc)
        {
            args.match.capture = false;
            args.match.playback = false;
            args.match.playback_or_capture = false;
            args.match.playback_and_capture = false;

            std::string typeStr = argv[i + 1];
            if (typeStr == "playback")
            {
                args.match.playback = true;
            }
            else if (typeStr == "capture")
            {
                args.match.capture = true;
            }
            else if (typeStr == "playback|capture" || typeStr == "playback | capture" ||
                typeStr == "capture|playback" || typeStr == "capture | playback")
            {
                args.match.capture = true;
                args.match.playback = true;
                args.match.playback_or_capture = true;
            }
            else if (typeStr == "playback&capture" || typeStr == "playback & capture" ||
                typeStr == "capture&playback" || typeStr == "capture & playback")
            {
                args.match.capture = true;
                args.match.playback = true;
                args.match.playback_and_capture = true;
            }
            args.args["--type"] = argv[i + 1];
            i++;
            continue;
        }
        else if (arg.find("--list") != std::string::npos)
        {
            args.list = true;
            args.args["--list"] = "";
        }
        else if (arg.find("--json") != std::string::npos)
        {
            args.json_output = true;
            args.args["--json"] = "";
        }
        else if (arg.find("--file") != std::string::npos && (i + 1) < argc)
        {
            args.file = argv[i + 1];
            args.print_to_file = true;
            args.args["--file"] = argv[i + 1];
            i++;
            continue;
        }
        else if (arg.find("--lang") != std::string::npos && (i + 1) < argc)
        {
            args.lang = argv[i + 1];
            args.args["--lang"] = argv[i + 1];
            i++;
            continue;
        }
        else if (arg.find("--lock") != std::string::npos)
        {
            args.lock = true;
            args.args["--lock"] = "";
        }
    }

    if (args.args.size() == 0)
    {
        args.list = true;
        args.verbose = true;
        args.match.playback = true;
        args.match.capture = true;
        args.match.playback_or_capture = true;
    }

    return args;
}

int list_devices(const command_line_args& args)
{
    if (args.verbose)
    {
        printf("%s", res.string("found_devices").c_str());
    }

    std::vector<audio_device> devices = get_audio_devices(args.match);

    if (args.json_output)
    {
        if (args.print_to_file)
        {
            if (std::filesystem::exists(args.file))
            {
                std::filesystem::remove(args.file);
            }
        }
        print_out(args, "%s\n", to_json_string(devices).c_str());
    }
    else
    {
        for (auto& d : devices)
        {
            if (!args.verbose || args.print_to_file)
            {
                print_out(args, "%s\n", d.plughw_id().c_str());
            }

            if (args.verbose)
            {
                printf("    %s\n", d.to_string().c_str());
            }

            if (!args.list)
            {
                break;
            }
        }
    }
    return (devices.size() > 0) ? 0 : 1;
}

int print_usage()
{
    printf("%s", res.string("help_all").c_str());
    return 1;
}

int lock()
{
    return 0;
}

int main(int argc, char* argv[])
{
    res.load();

    command_line_args args = parse_command_line_args(argc, argv);

    if (args.help)
    {
        return print_usage();
    }

    if (args.lock)
    {
        return lock();
    }

    return list_devices(args);
}