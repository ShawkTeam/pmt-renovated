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

# Generate build info (buildInfo.hpp.in)
function(generateBuildInfo BUILD_FLAGS)
	string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
	string(TIMESTAMP BUILD_TIME "%H:%M:%S")

	execute_process(COMMAND bash -c "mkdir -p ${CMAKE_SOURCE_DIR}/include/generated &>/dev/null")
	execute_process(COMMAND bash -c "${CMAKE_CXX_COMPILER} --version | head -n 1"
		OUTPUT_VARIABLE COMPILER_VERSION_STRING
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND bash -c "if which git &>/dev/null; then git rev-parse --short HEAD; else echo xxxxxxx; fi"
		OUTPUT_VARIABLE COMMIT_ID
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	configure_file(${CMAKE_SOURCE_DIR}/include/buildInfo.hpp.in ${CMAKE_SOURCE_DIR}/include/generated/buildInfo.hpp @ONLY)

	message(STATUS "Generated header: ${CMAKE_SOURCE_DIR}/include/generated/buildInfo.hpp")
endfunction()
