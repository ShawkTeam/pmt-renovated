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

target_link_options(pmt_shared_interface INTERFACE "-Wl,-rpath,/data/data/com.termux/files/usr/lib")
target_link_options(pmt_static_interface INTERFACE "-Wl,-rpath,/data/data/com.termux/files/usr/lib")
target_link_options(pmt_interface_nolibs INTERFACE "-Wl,-rpath,/data/data/com.termux/files/usr/lib")

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
