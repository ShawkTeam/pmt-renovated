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
function(generate_build_info BUILD_FLAGS)
	set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/include/generated")
	file(MAKE_DIRECTORY ${GENERATED_DIR})

	set(OUTPUT_FILE "${GENERATED_DIR}/buildInfo.hpp")
	set(INPUT_FILE "${CMAKE_SOURCE_DIR}/include/buildInfo.hpp.in")

	add_custom_command(
			OUTPUT ${OUTPUT_FILE}
			COMMAND ${CMAKE_COMMAND} -DOUTPUT_FILE=${OUTPUT_FILE}
			-DPROJECT_VERSION="${PROJECT_VERSION}"
			-DBUILD_FLAGS="${BUILD_FLAGS}"
			-DCMAKE_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
			-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
			-P "${CMAKE_SOURCE_DIR}/build/cmake/generate_headers_helper.cmake"
			VERBATIM
	)

	add_custom_target(
			generate_build_info ALL
			DEPENDS ${OUTPUT_FILE}
	)

	include_directories(${GENERATED_DIR}/..)

	get_property(ALL_TARGETS DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
	foreach(tgt IN LISTS ALL_TARGETS)
		add_dependencies(${tgt} generate_build_info)
	endforeach()
endfunction()
