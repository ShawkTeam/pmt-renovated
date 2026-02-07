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

# Define sources
set(PLUGIN_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/plugins/BackupPlugin.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/plugins/CleanLogPlugin.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/plugins/ErasePlugin.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/plugins/FlashPlugin.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/plugins/InfoPlugin.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/plugins/MemoryTestPlugin.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/plugins/PartitionSizePlugin.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/plugins/RealPathPlugin.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/plugins/RebootPlugin.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/plugins/TypePlugin.cpp
)

if(NOT BUILTIN_PLUGINS)
	set(PMT_SOURCES
			${CMAKE_CURRENT_SOURCE_DIR}/PartitionManager.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp)
else()
	set(PMT_SOURCES
			${CMAKE_CURRENT_SOURCE_DIR}/PartitionManager.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
			${PLUGIN_SOURCES})
	add_compile_definitions(BUILTIN_PLUGINS)
endif()

# Add pmt to build targets
add_executable(pmt ${PMT_SOURCES})
if(BUILTIN_PLUGINS)
	add_executable(pmt_static ${PMT_SOURCES})
else()
	add_executable(pmt_static ${PMT_SOURCES} ${PLUGIN_SOURCES})
	target_compile_definitions(pmt_static PRIVATE BUILTIN_PLUGINS)
endif()

# Apply common interfaces
target_link_libraries(pmt PRIVATE pmt_shared_interface)
target_link_libraries(pmt_static PRIVATE pmt_static_interface)

# Add libk directories for termux
target_link_directories(pmt PRIVATE "/data/data/com.termux/files/usr/lib")
target_link_directories(pmt_static PRIVATE "/data/data/com.termux/files/usr/lib")

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
