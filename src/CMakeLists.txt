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

# Define pmt sources.
set(PMT_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/PartitionManager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
)

# Add pmt to build targets
add_executable(pmt ${PMT_SOURCES})
add_executable(pmt_static ${PMT_SOURCES})

if(BUILTIN_PLUGINS)
	add_compile_definitions(BUILTIN_PLUGINS)
else()
	target_compile_definitions(pmt_static PRIVATE BUILTIN_PLUGINS)
endif()

# Apply common interfaces
target_link_libraries(pmt PRIVATE pmt::interface::shared)
target_link_libraries(pmt_static PRIVATE pmt::interface::static)

# Apply custom linker script
if(BUILTIN_PLUGINS)
	target_link_options(pmt PRIVATE "-Wl,-T,${CMAKE_SOURCE_DIR}/build/ld/builtin_modules.ld")
endif()
target_link_options(pmt_static PRIVATE "-Wl,-T,${CMAKE_SOURCE_DIR}/build/ld/builtin_modules.ld")

# Include plugins.
add_subdirectory(plugins)
