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

# Get list of .cpp files.
file(GLOB PLUGIN_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")

# Automatically plugin defination.
foreach(SOURCE_FILE ${PLUGIN_FILES})
    get_filename_component(RAW_NAME ${SOURCE_FILE} NAME_WLE) # Remove extension.
    string(REPLACE "Plugin" "" BASE_NAME ${RAW_NAME}) # Remove "Plugin" piece.
    string(REGEX REPLACE "([A-Z])" "_\\1" SNAKE_NAME ${BASE_NAME}) # Add an underscore (_) before capital letters.
    string(TOLOWER ${SNAKE_NAME} SNAKE_NAME)
    string(REGEX REPLACE "^_" "" FINAL_NAME ${SNAKE_NAME}) # Remove the leading underscore (_).
    add_plugin(${FINAL_NAME} SOURCES ${SOURCE_FILE})
endforeach()
