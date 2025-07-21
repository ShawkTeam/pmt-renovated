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

set(LIBHELPER_FLAGS -Wall -DLIB_BUILD_MODE)
set(LIBHELPER_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/Checkers.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Classes.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/FileUtil.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Sha256.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities.cpp
)

# Add targets
add_library(helper_shared SHARED ${LIBHELPER_SOURCES})
add_library(helper_static STATIC ${LIBHELPER_SOURCES})
add_executable(libhelper_test tests/test.cpp)

# Set compiler flags
target_compile_options(helper_shared PRIVATE ${LIBHELPER_FLAGS})
target_compile_options(helper_static PRIVATE ${LIBHELPER_FLAGS})
target_compile_options(libhelper_test PRIVATE ${LIBHELPER_FLAGS})
target_link_libraries(libhelper_test PRIVATE helper_shared)
target_link_options(libhelper_test PRIVATE "LINKER:-rpath,/data/data/com.termux/files/usr/lib")
target_link_options(helper_shared PRIVATE "LINKER:-rpath,/data/data/com.termux/files/usr/lib")

# Set appropriate output names
set_target_properties(helper_shared PROPERTIES OUTPUT_NAME "helper")
set_target_properties(helper_static PROPERTIES OUTPUT_NAME "helper")
