#
#  Copyright 2025 Yağız Zengin
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
	${CMAKE_CURRENT_SOURCE_DIR}/src/Getters.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/PartitionMap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Type.cpp
)
set(LIBPARTITION_MAP_FLAGS -DLOG_FILE="/storage/emulated/0/.last_pmt.log" -DPROGRAM_NAME="libpartition_map")

# Add targets
add_library(partition_map_shared SHARED ${LIBPARTITION_MAP_SOURCES})
add_library(partition_map_static STATIC ${LIBPARTITION_MAP_SOURCES})
add_executable(libpartition_map_test tests/test.cpp)

# Set appropriate output names
set_target_properties(partition_map_shared PROPERTIES OUTPUT_NAME "partition_map")
set_target_properties(partition_map_static PROPERTIES OUTPUT_NAME "partition_map")

# Set compiler flags
target_compile_options(partition_map_shared PRIVATE ${LIBPARTITION_MAP_FLAGS})
target_compile_options(partition_map_static PRIVATE ${LIBPARTITION_MAP_FLAGS})
target_compile_options(libpartition_map_test PRIVATE ${LIBPARTITION_MAP_FLAGS})
target_link_options(libpartition_map_test PRIVATE "LINKER:-rpath,/data/data/com.termux/files/usr/lib")
target_link_options(partition_map_shared PRIVATE "LINKER:-rpath,/data/data/com.termux/files/usr/lib")
target_link_libraries(libpartition_map_test PRIVATE partition_map_shared PRIVATE helper_shared)
target_link_libraries(partition_map_shared PRIVATE helper_shared)
