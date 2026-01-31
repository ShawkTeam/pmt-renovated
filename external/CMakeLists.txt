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

# Set directory paths
set(E2FSPROGS_LIB ${CMAKE_CURRENT_SOURCE_DIR}/e2fsprogs/lib)
set(GPTFDISK ${CMAKE_CURRENT_SOURCE_DIR}/gptfdisk)

# libext2_uuid sources
set(LIBEXT2_UUID_SRCS
        ${E2FSPROGS_LIB}/uuid/clear.c
        ${E2FSPROGS_LIB}/uuid/compare.c
        ${E2FSPROGS_LIB}/uuid/copy.c
        ${E2FSPROGS_LIB}/uuid/gen_uuid.c
        ${E2FSPROGS_LIB}/uuid/isnull.c
        ${E2FSPROGS_LIB}/uuid/pack.c
        ${E2FSPROGS_LIB}/uuid/parse.c
        ${E2FSPROGS_LIB}/uuid/unpack.c
        ${E2FSPROGS_LIB}/uuid/unparse.c
        ${E2FSPROGS_LIB}/uuid/uuid_time.c
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

# Add include directories
target_include_directories(libext2_uuid_shared PUBLIC ${E2FSPROGS_LIB} ${E2FSPROGS_LIB}/uuid)
target_include_directories(libext2_uuid_static PUBLIC ${E2FSPROGS_LIB} ${E2FSPROGS_LIB}/uuid)
target_include_directories(libgptf_shared PUBLIC ${GPTFDISK})
target_include_directories(libgptf_static PUBLIC ${GPTFDISK})

# Set appropriate output names
set_target_properties(libext2_uuid_shared PROPERTIES OUTPUT_NAME "ext2_uuid")
set_target_properties(libext2_uuid_static PROPERTIES OUTPUT_NAME "ext2_uuid")
set_target_properties(libgptf_static PROPERTIES OUTPUT_NAME "gptf")
set_target_properties(libgptf_shared PROPERTIES OUTPUT_NAME "gptf")

# Define required libraries
target_link_libraries(libgptf_static PRIVATE libext2_uuid_static)
target_link_libraries(libgptf_shared PRIVATE libext2_uuid_shared)

# Set compiler flags
target_compile_options(libext2_uuid_shared PRIVATE -Wno-pointer-arith -Wno-sign-compare -Wno-type-limits -Wno-typedef-redefinition -Wno-unused-parameter)
target_compile_options(libext2_uuid_static PRIVATE -Wno-pointer-arith -Wno-sign-compare -Wno-type-limits -Wno-typedef-redefinition -Wno-unused-parameter)
target_compile_options(libgptf_static PRIVATE -Wno-unused-parameter -Wno-macro-redefined -Wno-pragma-pack -std=c++11 -fno-strict-aliasing)
target_compile_options(libgptf_shared PRIVATE -Wno-unused-parameter -Wno-macro-redefined -Wno-pragma-pack -std=c++11 -fno-strict-aliasing)
target_compile_definitions(libgptf_static PRIVATE _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE _LARGEFILE64_SOURCE)
target_compile_definitions(libgptf_shared PRIVATE _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE _LARGEFILE64_SOURCE)

# Include CLI11 targets.
set(CLI11_SINGLE_FILE ON)
add_subdirectory(CLI11)
