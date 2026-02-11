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

# Include CLI11 targets.
set(CLI11_SINGLE_FILE ON)
add_subdirectory(CLI11)

# Set directory paths
set(E2FSPROGS ${CMAKE_CURRENT_SOURCE_DIR}/e2fsprogs)
set(GPTFDISK ${CMAKE_CURRENT_SOURCE_DIR}/gptfdisk)
set(PREBUILTS ${CMAKE_SOURCE_DIR}/build/prebuilts/${CMAKE_ANDROID_ARCH_ABI})

# libext2_uuid sources
set(LIBEXT2_UUID_SRCS
        ${E2FSPROGS}/lib/uuid/clear.c
        ${E2FSPROGS}/lib/uuid/compare.c
        ${E2FSPROGS}/lib/uuid/copy.c
        ${E2FSPROGS}/lib/uuid/gen_uuid.c
        ${E2FSPROGS}/lib/uuid/isnull.c
        ${E2FSPROGS}/lib/uuid/pack.c
        ${E2FSPROGS}/lib/uuid/parse.c
        ${E2FSPROGS}/lib/uuid/unpack.c
        ${E2FSPROGS}/lib/uuid/unparse.c
        ${E2FSPROGS}/lib/uuid/uuid_time.c
)

# libgptf sources
set(LIBGPTF_SRCS
        ${GPTFDISK}/gptcl.cc
        ${GPTFDISK}/crc32.cc
        ${GPTFDISK}/support.cc
        ${GPTFDISK}/guid.cc
        ${GPTFDISK}/gptpart.cc
        ${GPTFDISK}/mbrpart.cc
        ${GPTFDISK}/basicmbr.cc
        ${GPTFDISK}/mbr.cc
        ${GPTFDISK}/gpt.cc
        ${GPTFDISK}/bsd.cc
        ${GPTFDISK}/parttypes.cc
        ${GPTFDISK}/attributes.cc
        ${GPTFDISK}/diskio.cc
        ${GPTFDISK}/diskio-unix.cc
        ${GPTFDISK}/android_popt.cc
)

# Add libraries
add_library(libext2_uuid_shared SHARED ${LIBEXT2_UUID_SRCS})
add_library(libext2_uuid_static STATIC ${LIBEXT2_UUID_SRCS})
add_library(libgptf_static STATIC ${LIBGPTF_SRCS})
add_library(libgptf_shared SHARED ${LIBGPTF_SRCS})

# Set appropriate output names
set_target_properties(libext2_uuid_shared PROPERTIES OUTPUT_NAME "ext2_uuid")
set_target_properties(libext2_uuid_static PROPERTIES OUTPUT_NAME "ext2_uuid")
set_target_properties(libgptf_shared PROPERTIES OUTPUT_NAME "gptf")
set_target_properties(libgptf_static PROPERTIES OUTPUT_NAME "gptf")

# Define required libraries
target_link_libraries(libext2_uuid_shared PRIVATE pmt_interface_nolibs_nocflags)
target_link_libraries(libext2_uuid_static PRIVATE pmt_interface_nolibs_nocflags)
target_link_libraries(libgptf_shared PRIVATE pmt_interface_nolibs_nocflags libext2_uuid_shared)
target_link_libraries(libgptf_static PRIVATE pmt_interface_nolibs_nocflags libext2_uuid_static)

# Add compiler flags
target_compile_options(libext2_uuid_shared PRIVATE -Wno-pointer-arith -Wno-sign-compare -Wno-type-limits -Wno-typedef-redefinition -Wno-unused-parameter -fPIC)
target_compile_options(libext2_uuid_static PRIVATE -Wno-pointer-arith -Wno-sign-compare -Wno-type-limits -Wno-typedef-redefinition -Wno-unused-parameter -fPIC)
target_compile_options(libgptf_static PRIVATE -Wno-unused-parameter -Wno-macro-redefined -Wno-pragma-pack -std=c++11 -fno-strict-aliasing -fPIC)
target_compile_options(libgptf_shared PRIVATE -Wno-unused-parameter -Wno-macro-redefined -Wno-pragma-pack -std=c++11 -fno-strict-aliasing -fPIC)

# Add compiler definations
target_compile_definitions(libgptf_shared PRIVATE _FILE_OFFSET_BITS=64)
target_compile_definitions(libgptf_static PRIVATE _FILE_OFFSET_BITS=64)
