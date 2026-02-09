#
# Copyright (C) 2026 Yağız Zengin
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

# Project info
cmake_minimum_required(VERSION 3.20)
project(pmt VERSION 1.6.0)

# Use modern C++ 20 standart
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build plugins as built-in or dynamic.
option(BUILTIN_PLUGINS "Enable builtin plugins." OFF)

# Set compiler flags
add_compile_options(-Werror -Wno-deprecated-declarations)
if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	add_compile_options(-O3)
	add_link_options(-s)
endif()

# Add pmt's CMake module(s)
include(build/cmake/generate_headers.cmake)
include(build/cmake/interface.cmake)

# Generate header(s)
get_property(FLAGS DIRECTORY PROPERTY COMPILE_OPTIONS)
generateBuildInfo("${FLAGS}")

# Add libraries and executables
add_subdirectory(external)
add_subdirectory(srclib)
add_subdirectory(src)
