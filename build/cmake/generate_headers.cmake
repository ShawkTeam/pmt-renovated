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
