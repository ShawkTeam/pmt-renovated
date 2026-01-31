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

# Sources
set(PMT_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/FunctionManager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/PartitionManager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Out.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/functions/BackupFunction.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/functions/CleanLogFunction.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/functions/EraseFunction.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/functions/FlashFunction.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/functions/InfoFunction.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/functions/MemoryTestFunction.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/functions/PartitionSizeFunction.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/functions/RealPathFunction.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/functions/RebootFunction.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/functions/TypeFunction.cpp
)

# Add pmt
add_executable(pmt ${PMT_SOURCES})
add_executable(pmt_static ${PMT_SOURCES})

# Set linker options
target_link_libraries(pmt PRIVATE libhelper_shared libpartition_map_shared libgptf_shared libext2_uuid_shared CLI11_SINGLE)
target_link_libraries(pmt_static PRIVATE libhelper_static libpartition_map_static libgptf_static libext2_uuid_static CLI11_SINGLE)
target_link_options(pmt PRIVATE "LINKER:-rpath,/data/data/com.termux/files/usr/lib")
target_link_options(pmt_static PRIVATE "LINKER:-rpath,/data/data/com.termux/files/usr/lib")

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	target_link_options(pmt PRIVATE -fsanitize=address)
	target_link_options(pmt_static PRIVATE -fsanitize=address)
	target_compile_options(pmt PRIVATE -gdwarf-5 -fsanitize=address -fno-stack-protector)
	target_compile_options(pmt_static PRIVATE -gdwarf-5 -fsanitize=address -fno-stack-protector)
endif()
