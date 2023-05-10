# CMakeLists.txt
# Alsa audio device finding utility.
#
# MIT License
#
# Copyright (c) 2022 Ion Todirel
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required (VERSION 3.8)

project (find_devices VERSION 1.0 LANGUAGES CXX)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG master)
FetchContent_MakeAvailable(fmt)

add_executable (find_devices "find_devices.cpp" "find_devices.hpp" "main.cpp")

set_property(TARGET find_devices PROPERTY CXX_STANDARD 20)

#find_library(LIBUSB_LIBRARY NAMES usb-1.0 libusb-1.0)
#find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATH_SUFFIXES "include" "libusb" "libusb-1.0")
#find_library(LIBUSB_LIBRARY NAMES usb PATH_SUFFIXES "lib" "lib32" "lib64")
#target_include_directories(find_devices SYSTEM PUBLIC ${LIBUSB_INCLUDE_DIR})
#target_link_libraries(find_devices PUBLIC asound udev usb usb-1.0)

target_link_libraries(find_devices PUBLIC asound udev nlohmann_json::nlohmann_json fmt::fmt)

if (NOT UNIX)
    message(FATAL_ERROR "Only supported on Linux. Not a Linux operating system.")
endif (NOT UNIX)

set(GIT_HASH "unknown")

find_package(Git)

if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    )
endif()

message(STATUS "Git hash is ${GIT_HASH}")

target_compile_definitions(find_devices PRIVATE GIT_HASH=${GIT_HASH})

configure_file("${PROJECT_SOURCE_DIR}/config.json" "${PROJECT_BINARY_DIR}/config.json")
configure_file("${PROJECT_SOURCE_DIR}/config_schema.json" "${PROJECT_BINARY_DIR}/config_schema.json")