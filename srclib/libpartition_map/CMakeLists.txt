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

# Set linker flags and define required libraries
target_link_libraries(libpartition_map_shared PRIVATE pmt_interface_nolibs libhelper_shared libgptf_static libext2_uuid_static)
target_link_libraries(libpartition_map_static PRIVATE pmt_interface_nolibs libhelper_static libgptf_static libext2_uuid_static)
target_link_libraries(libpartition_map_test PRIVATE pmt_interface_nolibs libpartition_map_static libhelper_static libgptf_static libext2_uuid_static)

# Set compiler flags
target_compile_definitions(libpartition_map_shared PRIVATE _FILE_OFFSET_BITS=64)
target_compile_definitions(libpartition_map_static PRIVATE _FILE_OFFSET_BITS=64)
target_compile_definitions(libpartition_map_test PRIVATE _FILE_OFFSET_BITS=64)
