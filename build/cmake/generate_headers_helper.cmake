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

string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP BUILD_TIME "%H:%M:%S")

execute_process(
        COMMAND bash -c "${CMAKE_CXX_COMPILER} --version | head -n 1"
        OUTPUT_VARIABLE COMPILER_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
        COMMAND git rev-parse --short HEAD
        RESULT_VARIABLE GIT_OK
        OUTPUT_VARIABLE COMMIT_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT GIT_OK EQUAL 0)
    set(COMMIT_ID "xxxxxxx")
endif()

file(READ "${CMAKE_CURRENT_LIST_DIR}/../../include/buildInfo.hpp.in" HEADER_CONTENT)
string(REPLACE "@PROJECT_VERSION@" "${PROJECT_VERSION}" HEADER_CONTENT "${HEADER_CONTENT}")
string(REPLACE "@CMAKE_BUILD_TYPE@" "${CMAKE_BUILD_TYPE}" HEADER_CONTENT "${HEADER_CONTENT}")
string(REPLACE "@BUILD_DATE@" "${BUILD_DATE}" HEADER_CONTENT "${HEADER_CONTENT}")
string(REPLACE "@BUILD_TIME@" "${BUILD_TIME}" HEADER_CONTENT "${HEADER_CONTENT}")
string(REPLACE "@BUILD_CMAKE_VERSION@" "${CMAKE_VERSION}" HEADER_CONTENT "${HEADER_CONTENT}")
string(REPLACE "@BUILD_COMPILER_VERSION@" "${COMPILER_VERSION}" HEADER_CONTENT "${HEADER_CONTENT}")
string(REPLACE "@BUILD_FLAGS@" "${BUILD_FLAGS}" HEADER_CONTENT "${HEADER_CONTENT}")
string(REPLACE "@COMMIT_ID@" "${COMMIT_ID}" HEADER_CONTENT "${HEADER_CONTENT}")
string(REPLACE "\"\"" "\"" HEADER_CONTENT "${HEADER_CONTENT}")

file(WRITE "${OUTPUT_FILE}" "${HEADER_CONTENT}")
execute_process(COMMAND echo "Created ${OUTPUT_FILE}")
