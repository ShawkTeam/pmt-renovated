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
add_library(libpartition_map_shared SHARED ${LIBPARTITION_MAP_SOURCES})
add_library(libpartition_map_static STATIC ${LIBPARTITION_MAP_SOURCES})
add_executable(libpartition_map_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.cpp)

# Set appropriate output names
set_target_properties(libpartition_map_shared PROPERTIES OUTPUT_NAME "partition_map")
set_target_properties(libpartition_map_static PROPERTIES OUTPUT_NAME "partition_map")

# Apply common interface and define required libs
target_link_libraries(libpartition_map_shared PRIVATE pmt_interface_nolibs libhelper_shared libgptf_static libext2_uuid_static)
target_link_libraries(libpartition_map_static PRIVATE pmt_interface_nolibs libhelper_static libgptf_static libext2_uuid_static)
target_link_libraries(libpartition_map_test PRIVATE pmt_interface_nolibs libpartition_map_static libhelper_static libgptf_static libext2_uuid_static)

# Set compiler flags
target_compile_definitions(libpartition_map_shared PRIVATE _FILE_OFFSET_BITS=64)
target_compile_definitions(libpartition_map_static PRIVATE _FILE_OFFSET_BITS=64)
target_compile_definitions(libpartition_map_test PRIVATE _FILE_OFFSET_BITS=64)
