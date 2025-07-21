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
project(pmt VERSION 1.0.0)

set(DEBUG_FLAGS -gdwarf-5 -fsanitize=address -fstack-protector -O0)

# Add compiler flag based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEBUG_FLAGS}")
	set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${DEBUG_FLAGS}")
endif()

# Add include directories
include_directories(include srclib/libhelper/include srclib/libpartition_map/include)

# Add libraries
add_subdirectory(srclib/libhelper)
add_subdirectory(srclib/libpartition_map)

# NOTE: pmt is not ready, this CMakeFiles.txt only builds libraries
