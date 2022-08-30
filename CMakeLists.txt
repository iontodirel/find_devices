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

add_executable (find_devices "find_devices.cpp" "find_devices.h" "main.cpp")

set_property(TARGET find_devices PROPERTY CXX_STANDARD 20)

target_link_libraries(find_devices PUBLIC asound)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(find_devices PRIVATE -O0 -g -fno-aggressive-loop-optimizations)
endif()

configure_file("${PROJECT_SOURCE_DIR}/install_dependencies.sh" "${PROJECT_BINARY_DIR}/install_dependencies.sh")
configure_file("${PROJECT_SOURCE_DIR}/strings.en-us.txt" "${PROJECT_BINARY_DIR}/strings.en-us.txt")
configure_file("${PROJECT_SOURCE_DIR}/strings.de-de.txt" "${PROJECT_BINARY_DIR}/strings.de-de.txt")
configure_file("${PROJECT_SOURCE_DIR}/strings.it-it.txt" "${PROJECT_BINARY_DIR}/strings.it-it.txt")
