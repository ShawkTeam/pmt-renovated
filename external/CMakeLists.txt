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

# Set directory paths
set(E2FSPROGS ${CMAKE_CURRENT_SOURCE_DIR}/e2fsprogs)
set(F2FS_TOOLS ${CMAKE_CURRENT_SOURCE_DIR}/f2fs-tools)
set(EROFS_UTILS ${CMAKE_CURRENT_SOURCE_DIR}/erofs-utils)
set(GPTFDISK ${CMAKE_CURRENT_SOURCE_DIR}/gptfdisk)
set(FMTLIB ${CMAKE_CURRENT_SOURCE_DIR}/fmtlib)
set(LIBBASE ${CMAKE_CURRENT_SOURCE_DIR}/libbase)
set(LIBCUTILS ${CMAKE_CURRENT_SOURCE_DIR}/core/libcutils)
set(LIBCRYPTO_UTILS ${CMAKE_CURRENT_SOURCE_DIR}/core/libcrypto_utils)
set(LIBSPARSE ${CMAKE_CURRENT_SOURCE_DIR}/core/libsparse)
set(LIBEXT4_UTILS ${CMAKE_CURRENT_SOURCE_DIR}/extras/ext4_utils)
set(LIBLP ${CMAKE_CURRENT_SOURCE_DIR}/core/fs_mgr/liblp)
set(PATCHES ${CMAKE_CURRENT_SOURCE_DIR}/patches)

# Include BoringSSL's pre-created source list (for android)
set(BORINGSSL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/boringssl/)
include(boringssl/android-sources.cmake)

# Define include directories.
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/e2fsprogs/lib")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/e2fsprogs/lib/uuid")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/f2fs-tools/include")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/f2fs-tools/fsck")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/f2fs-tools/mkfs")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/gptfdisk")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/json/single_include")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/core/libcutils")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/core/libcutils/include")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/core/fs_mgr/include")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/core/libsparse/include")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/core/libcrypto_utils/include")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/core/fs_mgr/liblp/include")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/extras/ext4_utils/include")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/fmtlib/include")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/boringssl/src/include")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/patches/libbase/include")
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libbase/include")

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
        ${LIBBASE}/mapped_file.cpp
        ${LIBBASE}/parsebool.cpp
        ${LIBBASE}/parsenetaddress.cpp
        ${LIBBASE}/posix_strerror_r.cpp
        ${LIBBASE}/process.cpp
        ${LIBBASE}/result.cpp
        ${LIBBASE}/stringprintf.cpp
        ${LIBBASE}/strings.cpp
        ${LIBBASE}/threads.cpp
        ${LIBBASE}/test_utils.cpp
        ${LIBBASE}/errors_unix.cpp
        ${PATCHES}/libbase/properties.cpp
        ${PATCHES}/libbase/logging.cpp
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
add_super_library(ext2_uuid USE_INTERFACE_NOLIBS_NOFLAGS
        SOURCES ${LIBEXT2_UUID_SOURCES}
        COMPILE_OPTIONS -Wno-pointer-arith -Wno-sign-compare -Wno-type-limits -Wno-typedef-redefinition -Wno-unused-parameter
)
add_super_library(gptf USE_INTERFACE_NOLIBS_NOFLAGS
        SOURCES ${LIBGPTF_SOURCES}
        CPPSTD 17
        COMPILE_OPTIONS -Wno-unused-parameter -Wno-macro-redefined -Wno-pragma-pack -std=c++11 -fno-strict-aliasing
        DEFINATIONS _FILE_OFFSET_BITS=64
        LIBS ext2_uuid
)

if(ANDROID_NATIVE_API_LEVEL LESS_EQUAL 23)
    add_static_super_library(crypto_static USE_INTERFACE_NOLIBS_NOFLAGS
            SOURCES ${crypto_sources} ${crypto_sources_asm}
            CPPSTD 17
            COMPILE_OPTIONS -fvisibility=hidden -fno-data-sections -fno-function-sections -fno-sanitize=address -Werror -Wno-unused-parameter -Wno-reserved-user-defined-literal
            DEFINATIONS BORINGSSL_SHARED_LIBRARY BORINGSSL_ANDROID_SYSTEM BORINGSSL_IMPLEMENTATION OPENSSL_SMALL # There is no need for FIPS.
    )
    add_static_super_library(fmtlib_static USE_INTERFACE_NOLIBS_NOFLAGS NOPREFIX
            SOURCES ${FMTLIB_SOURCES}
            CPPSTD 17
            COMPILE_OPTIONS -fno-exceptions -UNDEBUG
    )
    add_static_super_library(base_static USE_INTERFACE_NOLIBS_NOFLAGS
            SOURCES ${LIBBASE_SOURCES}
            CPPSTD 20
            COMPILE_OPTIONS -Wexit-time-destructors -Wno-error
            DEFINATIONS _FILE_OFFSET_BITS=64
            LIBS fmtlib_static log
    )
    add_static_super_library(crypto_utils_static USE_INTERFACE_NOLIBS_NOFLAGS
            SOURCES ${LIBCRYPTO_UTILS_SOURCES}
            CPPSTD 17
            LIBS libcrypto_static
    )
    add_static_super_library(sparse_static USE_INTERFACE_NOLIBS_NOFLAGS
            SOURCES ${LIBSPARSE_SOURCES}
            CPPSTD 17
            COMPILE_OPTIONS -Wno-macro-redefined
            LIBS z
    )
    add_static_super_library(ext4_utils_static USE_INTERFACE_NOLIBS_NOFLAGS
            SOURCES ${LIBEXT4_UTILS_SOURCES}
            CPPSTD 17
            COMPILE_OPTIONS -fno-strict-aliasing
            DEFINATIONS _FILE_OFFSET_BITS=64 REAL_UUID
            LIBS ext2_uuid
    )
    add_static_super_library(lp_static USE_INTERFACE_NOLIBS_NOFLAGS
            SOURCES ${LIBLP_SOURCES}
            CPPSTD 17
            COMPILE_OPTIONS -Wno-error
            DEFINATIONS _FILE_OFFSET_BITS=64
            LIBS libbase_static libcrypto_utils_static libcrypto_static libsparse_static libext4_utils_static
    )
endif()

if(ANDROID_NATIVE_API_LEVEL GREATER_EQUAL 30)
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set_source_files_properties(
                ${crypto_sources_asm}
                PROPERTIES
                COMPILE_FLAGS "-fsanitize=hwaddress"
        )
    endif()

    add_shared_super_library(crypto USE_INTERFACE_NOLIBS_NOFLAGS
            SOURCES ${crypto_sources} ${crypto_sources_asm}
            CPPSTD 17
            COMPILE_OPTIONS -fvisibility=hidden -fno-data-sections -fno-function-sections -fno-sanitize=address -Werror -Wno-unused-parameter -Wno-reserved-user-defined-literal
            LINKER_OPTIONS -Wl,-Bsymbolic
            DEFINATIONS BORINGSSL_SHARED_LIBRARY BORINGSSL_ANDROID_SYSTEM BORINGSSL_IMPLEMENTATION OPENSSL_SMALL # There is no need for FIPS.
    )
    add_static_super_library(fmtlib_static USE_INTERFACE_NOLIBS_NOFLAGS NOPREFIX
            SOURCES ${FMTLIB_SOURCES}
            CPPSTD 17
            COMPILE_OPTIONS -fno-exceptions -UNDEBUG
    )
    add_static_super_library(base_static USE_INTERFACE_NOLIBS_NOFLAGS
            SOURCES ${LIBBASE_SOURCES}
            CPPSTD 20
            COMPILE_OPTIONS -Wexit-time-destructors -Wno-error
            DEFINATIONS _FILE_OFFSET_BITS=64
            LIBS fmtlib_static log
    )
    add_shared_super_library(crypto_utils USE_INTERFACE_NOLIBS_NOFLAGS
            SOURCES ${LIBCRYPTO_UTILS_SOURCES}
            CPPSTD 17
            LIBS libcrypto
    )
    add_shared_super_library(sparse USE_INTERFACE_NOLIBS_NOFLAGS
            SOURCES ${LIBSPARSE_SOURCES}
            CPPSTD 17
            COMPILE_OPTIONS -Wno-macro-redefined
            LIBS libbase_static z
    )
    add_shared_super_library(ext4_utils USE_INTERFACE_NOLIBS_NOFLAGS
            SOURCES ${LIBEXT4_UTILS_SOURCES}
            CPPSTD 17
            COMPILE_OPTIONS -fno-strict-aliasing
            DEFINATIONS _FILE_OFFSET_BITS=64 REAL_UUID
            LIBS libbase_static ext2_uuid
    )
    add_static_super_library(lp_static USE_INTERFACE_NOLIBS_NOFLAGS
            SOURCES ${LIBLP_SOURCES}
            CPPSTD 17
            COMPILE_OPTIONS -Wno-error
            DEFINATIONS _FILE_OFFSET_BITS=64
            LIBS libbase_static libcrypto_utils libcrypto libsparse libext4_utils
    )
endif()
