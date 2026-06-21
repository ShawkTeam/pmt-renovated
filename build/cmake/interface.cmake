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

# Export an empty include dir list.
set_property(GLOBAL PROPERTY PMT_INCLUDE_DIRECTORIES "")

target_link_options(pmt_interface_shared INTERFACE "-Wl,-rpath,/data/data/com.termux/files/usr/lib" "-Wl,--hash-style=both")
target_link_options(pmt_interface_static INTERFACE "-Wl,-rpath,/data/data/com.termux/files/usr/lib" "-Wl,--hash-style=both")
target_link_options(pmt_interface_nolibs INTERFACE "-Wl,-rpath,/data/data/com.termux/files/usr/lib" "-Wl,--hash-style=both")
target_link_options(pmt_interface_nolibs_and_flags INTERFACE "-Wl,-rpath,/data/data/com.termux/files/usr/lib" "-Wl,--hash-style=both")

target_link_libraries(pmt_interface_shared INTERFACE libhelper_shared libpartition_map_shared libgptf_static libext2_uuid_static)
target_link_libraries(pmt_interface_static INTERFACE libhelper_static libpartition_map_static libgptf_static libext2_uuid_static)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_link_options(pmt_interface_shared INTERFACE -fsanitize=address)
    target_link_options(pmt_interface_static INTERFACE -fsanitize=address)
    target_link_options(pmt_interface_nolibs INTERFACE -fsanitize=address)
    
    target_compile_options(pmt_interface_shared INTERFACE -gdwarf-5 -fsanitize=address -fno-stack-protector)
    target_compile_options(pmt_interface_static INTERFACE -gdwarf-5 -fsanitize=address -fno-stack-protector)
    target_compile_options(pmt_interface_nolibs INTERFACE -gdwarf-5 -fsanitize=address -fno-stack-protector)
endif()

if(LINK_TIME_OPTIMIZATION_THIN)
    target_compile_options(pmt_interface_shared INTERFACE -flto=thin)
    target_compile_options(pmt_interface_static INTERFACE -flto=thin)
    target_compile_options(pmt_interface_nolibs INTERFACE -flto=thin)
    target_link_options(pmt_interface_shared INTERFACE -flto=thin)
    target_link_options(pmt_interface_static INTERFACE -flto=thin)
    target_link_options(pmt_interface_nolibs INTERFACE -flto=thin)
endif()

if(ANDROID_NATIVE_API_LEVEL LESS_EQUAL 23)
    target_compile_options(pmt_interface_shared INTERFACE -U_FILE_OFFSET_BITS -D_FILE_OFFSET_BITS=32)
    target_compile_options(pmt_interface_static INTERFACE -U_FILE_OFFSET_BITS -D_FILE_OFFSET_BITS=32)
    target_compile_options(pmt_interface_nolibs INTERFACE -U_FILE_OFFSET_BITS -D_FILE_OFFSET_BITS=32)
    target_compile_options(pmt_interface_nolibs_and_flags INTERFACE -U_FILE_OFFSET_BITS -D_FILE_OFFSET_BITS=32)
endif()

function(init_include_directories)
    get_property(dirs GLOBAL PROPERTY PMT_INCLUDE_DIRECTORIES)
    foreach(target pmt_interface_shared pmt_interface_static pmt_interface_nolibs pmt_interface_nolibs_and_flags)
        target_include_directories(${target} INTERFACE ${dirs})
    endforeach()
endfunction()
