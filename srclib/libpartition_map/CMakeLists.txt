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
set(LIBPARTITION_MAP_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/Builder.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Magic.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Partition.cpp
)

# Add targets
add_super_library(partition_map USE_INTERFACE_NOLIBS
		SOURCES ${LIBPARTITION_MAP_SOURCES}
		DEFINATIONS _FILE_OFFSET_BITS=64
		LIBS helper ext2_uuid
		MANUAL_LIBS_OF_SHARED libgptf_static
		MANUAL_LIBS_OF_STATIC libgptf_static
)

# Add tests
add_executable(libpartition_map_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.cpp)

# Apply common interface and define required libs to tests
target_link_libraries(libpartition_map_test PRIVATE pmt::interface::nolibs libpartition_map_static libhelper_static libgptf_static libext2_uuid_static)

# Set compiler flags for tests
target_compile_definitions(libpartition_map_test PRIVATE _FILE_OFFSET_BITS=64)
