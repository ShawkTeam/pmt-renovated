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
if(NOT BUILTIN_PLUGINS)
	set(PMT_SOURCES
			${CMAKE_CURRENT_SOURCE_DIR}/PartitionManager.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp)
else()
	set(PMT_SOURCES
			${CMAKE_CURRENT_SOURCE_DIR}/PartitionManager.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/plugins/BackupPlugin.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/plugins/CleanLogPlugin.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/plugins/ErasePlugin.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/plugins/FlashPlugin.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/plugins/InfoPlugin.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/plugins/MemoryTestPlugin.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/plugins/PartitionSizePlugin.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/plugins/RealPathPlugin.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/plugins/RebootPlugin.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/plugins/TypePlugin.cpp)
	add_compile_definitions(BUILTIN_PLUGINS)
endif()

# Add pmt
add_executable(pmt ${PMT_SOURCES})
add_executable(pmt_static ${PMT_SOURCES})

# Set linker options
target_link_libraries(pmt PRIVATE pmt_shared_interface)
target_link_libraries(pmt_static PRIVATE pmt_static_interface)

if(NOT BUILTIN_PLUGINS)
	add_library(pmt_backup_plugin SHARED ${CMAKE_CURRENT_SOURCE_DIR}/plugins/BackupPlugin.cpp)
	add_library(pmt_clean_log_plugin SHARED ${CMAKE_CURRENT_SOURCE_DIR}/plugins/CleanLogPlugin.cpp)
	add_library(pmt_erase_plugin SHARED ${CMAKE_CURRENT_SOURCE_DIR}/plugins/ErasePlugin.cpp)
	add_library(pmt_flash_plugin SHARED ${CMAKE_CURRENT_SOURCE_DIR}/plugins/FlashPlugin.cpp)
	add_library(pmt_info_plugin SHARED ${CMAKE_CURRENT_SOURCE_DIR}/plugins/InfoPlugin.cpp)
	add_library(pmt_memory_test_plugin SHARED ${CMAKE_CURRENT_SOURCE_DIR}/plugins/MemoryTestPlugin.cpp)
	add_library(pmt_partition_size_plugin SHARED ${CMAKE_CURRENT_SOURCE_DIR}/plugins/PartitionSizePlugin.cpp)
	add_library(pmt_real_path_plugin SHARED ${CMAKE_CURRENT_SOURCE_DIR}/plugins/RealPathPlugin.cpp)
	add_library(pmt_reboot_plugin SHARED ${CMAKE_CURRENT_SOURCE_DIR}/plugins/RebootPlugin.cpp)
	add_library(pmt_type_plugin SHARED ${CMAKE_CURRENT_SOURCE_DIR}/plugins/TypePlugin.cpp)
	target_link_libraries(pmt_backup_plugin PRIVATE pmt_shared_interface)
	target_link_libraries(pmt_clean_log_plugin PRIVATE pmt_shared_interface)
	target_link_libraries(pmt_erase_plugin PRIVATE pmt_shared_interface)
	target_link_libraries(pmt_flash_plugin PRIVATE pmt_shared_interface)
	target_link_libraries(pmt_info_plugin PRIVATE pmt_shared_interface)
	target_link_libraries(pmt_memory_test_plugin PRIVATE pmt_shared_interface)
	target_link_libraries(pmt_partition_size_plugin PRIVATE pmt_shared_interface)
	target_link_libraries(pmt_real_path_plugin PRIVATE pmt_shared_interface)
	target_link_libraries(pmt_reboot_plugin PRIVATE pmt_shared_interface)
	target_link_libraries(pmt_type_plugin PRIVATE pmt_shared_interface)
	set_target_properties(pmt_backup_plugin PROPERTIES PREFIX "" OUTPUT_NAME "backupplugin")
	set_target_properties(pmt_clean_log_plugin PROPERTIES PREFIX "" OUTPUT_NAME "cleanlogplugin")
	set_target_properties(pmt_erase_plugin PROPERTIES PREFIX "" OUTPUT_NAME "eraseplugin")
	set_target_properties(pmt_flash_plugin PROPERTIES PREFIX "" OUTPUT_NAME "flashplugin")
	set_target_properties(pmt_info_plugin PROPERTIES PREFIX "" OUTPUT_NAME "infoplugin")
	set_target_properties(pmt_memory_test_plugin PROPERTIES PREFIX "" OUTPUT_NAME "memorytestplugin")
	set_target_properties(pmt_partition_size_plugin PROPERTIES PREFIX "" OUTPUT_NAME "partitionsizeplugin")
	set_target_properties(pmt_real_path_plugin PROPERTIES PREFIX "" OUTPUT_NAME "realpathplugin")
	set_target_properties(pmt_reboot_plugin PROPERTIES PREFIX "" OUTPUT_NAME "rebootplugin")
	set_target_properties(pmt_type_plugin PROPERTIES PREFIX "" OUTPUT_NAME "typeplugin")
endif()
