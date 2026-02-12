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
set(FMTLIB ${CMAKE_CURRENT_SOURCE_DIR}/fmtlib)
set(LIBBASE ${CMAKE_CURRENT_SOURCE_DIR}/libbase)
set(LIBCUTILS ${CMAKE_CURRENT_SOURCE_DIR}/core/libcutils)
set(LIBCRYPTO_UTILS ${CMAKE_CURRENT_SOURCE_DIR}/core/libcrypto_utils)
set(LIBSPARSE ${CMAKE_CURRENT_SOURCE_DIR}/core/libsparse)
set(LIBEXT4_UTILS ${CMAKE_CURRENT_SOURCE_DIR}/extras/ext4_utils)
set(LIBLP ${CMAKE_CURRENT_SOURCE_DIR}/core/fs_mgr/liblp)
set(PREBUILTS ${CMAKE_SOURCE_DIR}/build/prebuilts/${CMAKE_ANDROID_ARCH_ABI})

# libext2_uuid sources
set(LIBEXT2_UUID_SOURCES
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
set(LIBGPTF_SOURCES
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

# fmtlib sources
set(FMTLIB_SOURCES ${FMTLIB}/src/format.cc)

# libbase sources
set(LIBBASE_SOURCES
        ${LIBBASE}/abi_compatibility.cpp
        ${LIBBASE}/chrono_utils.cpp
        ${LIBBASE}/cmsg.cpp
        ${LIBBASE}/file.cpp
        ${LIBBASE}/hex.cpp
        ${LIBBASE}/logging.cpp
        ${LIBBASE}/mapped_file.cpp
        ${LIBBASE}/parsebool.cpp
        ${LIBBASE}/parsenetaddress.cpp
        ${LIBBASE}/posix_strerror_r.cpp
        ${LIBBASE}/process.cpp
        ${LIBBASE}/properties.cpp
        ${LIBBASE}/result.cpp
        ${LIBBASE}/stringprintf.cpp
        ${LIBBASE}/strings.cpp
        ${LIBBASE}/threads.cpp
        ${LIBBASE}/test_utils.cpp
        ${LIBBASE}/errors_unix.cpp
        ${LIBCUTILS}/android_get_control_file.cpp
)

# libcrypto_utils sources
set(LIBCRYPTO_UTILS_SOURCES ${LIBCRYPTO_UTILS}/android_pubkey.cpp)

# libsparse sources
set(LIBSPARSE_SOURCES
        ${LIBSPARSE}/backed_block.cpp
        ${LIBSPARSE}/output_file.cpp
        ${LIBSPARSE}/sparse.cpp
        ${LIBSPARSE}/sparse_crc32.cpp
        ${LIBSPARSE}/sparse_err.cpp
        ${LIBSPARSE}/sparse_read.cpp
)

# libext4_utils sources
set(LIBEXT4_UTILS_SOURCES
        ${LIBEXT4_UTILS}/ext4_utils.cpp
        ${LIBEXT4_UTILS}/wipe.cpp
        ${LIBEXT4_UTILS}/ext4_sb.cpp
)

# liblp sources
set(LIBLP_SOURCES
        ${LIBLP}/builder.cpp
        ${LIBLP}/super_layout_builder.cpp
        ${LIBLP}/images.cpp
        ${LIBLP}/partition_opener.cpp
        ${LIBLP}/property_fetcher.cpp
        ${LIBLP}/reader.cpp
        ${LIBLP}/utility.cpp
        ${LIBLP}/writer.cpp
)

# Add libraries
add_library(libcrypto SHARED IMPORTED)
set_target_properties(libcrypto PROPERTIES IMPORTED_LOCATION "${PREBUILTS}/libcrypto.so")

add_super_library(ext2_uuid USE_INTERFACE_NOLIBS_NOFLAGS
        SOURCES ${LIBEXT2_UUID_SOURCES}
        COMPILE_OPTIONS -Wno-pointer-arith -Wno-sign-compare -Wno-type-limits -Wno-typedef-redefinition -Wno-unused-parameter
)
add_super_library(gptf USE_INTERFACE_NOLIBS_NOFLAGS
        SOURCES ${LIBGPTF_SOURCES}
        COMPILE_OPTIONS -Wno-unused-parameter -Wno-macro-redefined -Wno-pragma-pack -std=c++11 -fno-strict-aliasing
        DEFINATIONS _FILE_OFFSET_BITS=64
        LIBS ext2_uuid
)
add_super_library(fmtlib USE_INTERFACE_NOLIBS_NOFLAGS NOPREFIX
        SOURCES ${FMTLIB_SOURCES}
        COMPILE_OPTIONS -fno-exceptions -UNDEBUG
)
add_super_library(base USE_INTERFACE_NOLIBS_NOFLAGS
        SOURCES ${LIBBASE_SOURCES}
        COMPILE_OPTIONS -Wexit-time-destructors -Wno-error
        DEFINATIONS _FILE_OFFSET_BITS=64
        LIBS fmtlib log
)
add_super_library(crypto_utils USE_INTERFACE_NOLIBS_NOFLAGS
        SOURCES ${LIBCRYPTO_UTILS_SOURCES}
        LIBS libcrypto
)
add_super_library(sparse USE_INTERFACE_NOLIBS_NOFLAGS
        SOURCES ${LIBSPARSE_SOURCES}
        LIBS base z
)
add_super_library(ext4_utils USE_INTERFACE_NOLIBS_NOFLAGS
        SOURCES ${LIBEXT4_UTILS_SOURCES}
        COMPILE_OPTIONS -fno-strict-aliasing
        DEFINATIONS _FILE_OFFSET_BITS=64 REAL_UUID
        LIBS base ext2_uuid
)
add_super_library(lp USE_INTERFACE_NOLIBS_NOFLAGS
        SOURCES ${LIBLP_SOURCES}
        COMPILE_OPTIONS -Wno-error
        DEFINATIONS _FILE_OFFSET_BITS=64
        LIBS base crypto_utils libcrypto sparse ext4_utils
)
