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
add_library(pmt_interface_shared INTERFACE)
add_library(pmt_interface_static INTERFACE)
add_library(pmt_interface_nolibs INTERFACE)
add_library(pmt_interface_nolibs_and_flags INTERFACE)

add_library(pmt::interface::shared ALIAS pmt_interface_shared)
add_library(pmt::interface::static ALIAS pmt_interface_static)
add_library(pmt::interface::nolibs ALIAS pmt_interface_nolibs)
add_library(pmt::interface::nolibs_and_flags ALIAS pmt_interface_nolibs_and_flags)

set(INCLUDE_DIRECTORIES
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/external/e2fsprogs/lib"
        "${CMAKE_SOURCE_DIR}/external/e2fsprogs/lib/uuid"
        "${CMAKE_SOURCE_DIR}/external/gptfdisk"
        "${CMAKE_SOURCE_DIR}/external/json/single_include"
        "${CMAKE_SOURCE_DIR}/external/picosha2"
        "${CMAKE_SOURCE_DIR}/external/core/libcutils/include"
        "${CMAKE_SOURCE_DIR}/external/core/libcutils"
        "${CMAKE_SOURCE_DIR}/external/core/fs_mgr/include"
        "${CMAKE_SOURCE_DIR}/external/core/fs_mgr/liblp/include"
        "${CMAKE_SOURCE_DIR}/external/core/libsparse/include"
        "${CMAKE_SOURCE_DIR}/external/core/libcrypto_utils/include"
        "${CMAKE_SOURCE_DIR}/external/libbase/include"
        "${CMAKE_SOURCE_DIR}/external/extras/ext4_utils/include"
        "${CMAKE_SOURCE_DIR}/external/fmtlib/include"
        "${CMAKE_SOURCE_DIR}/external/boringssl/src/include"
        "${CMAKE_SOURCE_DIR}/srclib/libhelper/include"
        "${CMAKE_SOURCE_DIR}/srclib/libpartition_map/include"
)

target_link_options(pmt_interface_shared INTERFACE "-Wl,-rpath,/data/data/com.termux/files/usr/lib")
target_link_options(pmt_interface_static INTERFACE "-Wl,-rpath,/data/data/com.termux/files/usr/lib")
target_link_options(pmt_interface_nolibs INTERFACE "-Wl,-rpath,/data/data/com.termux/files/usr/lib")
target_link_options(pmt_interface_nolibs_and_flags INTERFACE "-Wl,-rpath,/data/data/com.termux/files/usr/lib")

target_link_libraries(pmt_interface_shared INTERFACE libhelper_shared libpartition_map_shared libgptf_static libext2_uuid_static CLI11_SINGLE)
target_link_libraries(pmt_interface_static INTERFACE libhelper_static libpartition_map_static libgptf_static libext2_uuid_static CLI11_SINGLE)

target_include_directories(pmt_interface_shared INTERFACE ${INCLUDE_DIRECTORIES})
target_include_directories(pmt_interface_static INTERFACE ${INCLUDE_DIRECTORIES})
target_include_directories(pmt_interface_nolibs INTERFACE ${INCLUDE_DIRECTORIES})
target_include_directories(pmt_interface_nolibs_and_flags INTERFACE ${INCLUDE_DIRECTORIES})

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_link_options(pmt_interface_shared INTERFACE -fsanitize=address)
    target_link_options(pmt_interface_static INTERFACE -fsanitize=address)
    target_link_options(pmt_interface_nolibs INTERFACE -fsanitize=address)
    
    target_compile_options(pmt_interface_shared INTERFACE -gdwarf-5 -fsanitize=address -fno-stack-protector)
    target_compile_options(pmt_interface_static INTERFACE -gdwarf-5 -fsanitize=address -fno-stack-protector)
    target_compile_options(pmt_interface_nolibs INTERFACE -gdwarf-5 -fsanitize=address -fno-stack-protector)
endif()
