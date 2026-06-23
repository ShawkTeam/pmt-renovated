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

# Sources
set(LIBOPENPART_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/core.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/gpt.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/io.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/mount.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/utility.c
)

# Define include directories
add_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_super_library(openpart USE_INTERFACE_NOLIBS
		SOURCES ${LIBOPENPART_SOURCES}
		LIBS ext2_uuid
		MANUAL_LIBS z $<IF:$<VERSION_LESS:${ANDROID_NATIVE_API_LEVEL},23>,libcrypto_static,libcrypto>
)

# Add tests
add_executable(openpart_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.c)
target_link_libraries(openpart_test PRIVATE pmt::interface::nolibs libopenpart_static)
