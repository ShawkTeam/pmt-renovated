#
#  Copyright 2026 Yağız Zengin
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
