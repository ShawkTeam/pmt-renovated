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

# Add common interface for libraries, etc.
add_library(pmt_shared_interface INTERFACE)
add_library(pmt_static_interface INTERFACE)
add_library(pmt_interface_nolibs INTERFACE)

set(INCLUDE_DIRECTORIES
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/external/e2fsprogs/lib"
        "${CMAKE_SOURCE_DIR}/external/e2fsprogs/uuid"
        "${CMAKE_SOURCE_DIR}/external/gptfdisk"
        "${CMAKE_SOURCE_DIR}/external/json/single_include"
        "${CMAKE_SOURCE_DIR}/external/picosha2"
        "${CMAKE_SOURCE_DIR}/external/core/libcutils/include"
        "${CMAKE_SOURCE_DIR}/srclib/libhelper/include"
        "${CMAKE_SOURCE_DIR}/srclib/libpartition_map/include"
)

target_link_directories(pmt_shared_interface INTERFACE "/data/data/com.termux/files/usr/lib")
target_link_directories(pmt_static_interface INTERFACE "/data/data/com.termux/files/usr/lib")
target_link_directories(pmt_interface_nolibs INTERFACE "/data/data/com.termux/files/usr/lib")

target_link_libraries(pmt_shared_interface INTERFACE libhelper_shared libpartition_map_shared libgptf_static libext2_uuid_static CLI11_SINGLE)
target_link_libraries(pmt_static_interface INTERFACE libhelper_static libpartition_map_static libgptf_static libext2_uuid_static CLI11_SINGLE)

target_include_directories(pmt_shared_interface INTERFACE ${INCLUDE_DIRECTORIES})
target_include_directories(pmt_static_interface INTERFACE ${INCLUDE_DIRECTORIES})
target_include_directories(pmt_interface_nolibs INTERFACE ${INCLUDE_DIRECTORIES})

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_link_options(pmt_shared_interface INTERFACE -fsanitize=address)
    target_link_options(pmt_static_interface INTERFACE -fsanitize=address)
    target_link_options(pmt_interface_nolibs INTERFACE -fsanitize=address)
    
    target_compile_options(pmt_shared_interface INTERFACE -gdwarf-5 -fsanitize=address -fno-stack-protector)
    target_compile_options(pmt_static_interface INTERFACE -gdwarf-5 -fsanitize=address -fno-stack-protector)
    target_compile_options(pmt_interface_nolibs INTERFACE -gdwarf-5 -fsanitize=address -fno-stack-protector)
endif()
