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
set(LIBHELPER_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/Checkers.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Classes.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/FileUtil.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Sha256.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities.cpp
)

# Add targets
add_library(libhelper_shared SHARED ${LIBHELPER_SOURCES})
add_library(libhelper_static STATIC ${LIBHELPER_SOURCES})
add_executable(libhelper_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.cpp)

# Set linker flags and define rquired libraries
target_link_libraries(libhelper_shared PRIVATE pmt_interface_nolibs)
target_link_libraries(libhelper_static PRIVATE pmt_interface_nolibs)
target_link_libraries(libhelper_test PRIVATE pmt_interface_nolibs libhelper_static)

# Set appropriate output names and compiler flags
set_target_properties(libhelper_shared PROPERTIES OUTPUT_NAME "helper")
set_target_properties(libhelper_static PROPERTIES OUTPUT_NAME "helper")
