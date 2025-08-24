#
#  Copyright 2025 Yağız Zengin
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
cmake_minimum_required(VERSION 3.10)
project(pmt VERSION 1.2.0)

# Set compiler flags
add_compile_options(-Wall -Werror -Wno-deprecated-declarations)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	add_compile_options(-gdwarf-5 -fsanitize=address -fstack-protector)
	add_link_options(-fsanitize=address)
else()
	add_compile_options(-Os)
	add_link_options(-s)
endif()

# Add pmt's CMake module(s)
include(build/cmake/generate_headers.cmake)

# Generate header(s)
get_property(FLAGS DIRECTORY PROPERTY COMPILE_OPTIONS)
generateBuildInfo("${FLAGS}")

# Add include directories
include_directories(include srclib/libhelper/include srclib/libpartition_map/include)

# Add libraries
add_subdirectory(srclib)

# Include pmt
add_subdirectory(src)
