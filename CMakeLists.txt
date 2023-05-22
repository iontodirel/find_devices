# **************************************************************** #
# find_devices - Audio device and serial ports search utility      #
# Version 0.1.0                                                    #
# https://github.com/iontodirel/find_devices                       #
# Copyright (c) 2023 Ion Todirel                                   #
# **************************************************************** #
#
# CMakeLists.txt
# Audio device and serial ports finding utility.
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
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG master)
FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
  FetchContent_Populate(fmt)
  add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(cxxopts GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git GIT_TAG v3.1.1)
FetchContent_GetProperties(cxxopts)
if(NOT cxxopts_POPULATED)
  FetchContent_Populate(cxxopts)
  add_subdirectory(${cxxopts_SOURCE_DIR} ${cxxopts_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_executable (find_devices "find_devices.cpp" "find_devices.hpp" "main.cpp")

set_property(TARGET find_devices PROPERTY CXX_STANDARD 20)

if (NOT UNIX)
    message(FATAL_ERROR "Only supported on Linux. Not a Linux operating system.")
endif (NOT UNIX)

set(GIT_HASH "unknown")
set(FIND_DEVICES_VERSION "0.1.0")

find_package(Git)

if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
endif()

message(STATUS "Git hash: ${GIT_HASH}")

target_compile_definitions(find_devices PRIVATE GIT_HASH=${GIT_HASH} FIND_DEVICES_VERSION=${FIND_DEVICES_VERSION})
if (${ENABLE_WARNINGS_AS_ERRORS})
  target_compile_options(find_devices PRIVATE -v -Wall -Wextra -Wpedantic -Werror)
  message("ENABLE_WARNINGS_AS_ERRORS is ON")
endif()
target_link_libraries(find_devices PRIVATE asound udev nlohmann_json::nlohmann_json fmt::fmt cxxopts)

configure_file("${PROJECT_SOURCE_DIR}/config.json" "${PROJECT_BINARY_DIR}/config.json")
configure_file("${PROJECT_SOURCE_DIR}/config_schema.json" "${PROJECT_BINARY_DIR}/config_schema.json")

install(FILES ${PROJECT_BINARY_DIR}/find_devices PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ DESTINATION bin)
install(FILES ${PROJECT_BINARY_DIR}/config.json PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ DESTINATION bin)
install(FILES ${PROJECT_BINARY_DIR}/config_schema.json PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ DESTINATION bin)