#
#  Copyright 2026 Yağız Zengin
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

# Project info
cmake_minimum_required(VERSION 3.20)
project(pmt VERSION 1.5.0)

# Use modern C++ 20 standart
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compiler flags
add_compile_options(-Werror -Wno-deprecated-declarations)
if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	add_compile_options(-O3)
	add_link_options(-s)
endif()

# Add pmt's CMake module(s)
include(build/cmake/generate_headers.cmake)

# Generate header(s)
get_property(FLAGS DIRECTORY PROPERTY COMPILE_OPTIONS)
generateBuildInfo("${FLAGS}")

# Add include directories
include_directories(include)

# Add libraries and executables
add_subdirectory(external)
add_subdirectory(srclib)
add_subdirectory(src)
