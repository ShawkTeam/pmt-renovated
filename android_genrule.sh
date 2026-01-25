#!/usr/bin/bash
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

echo -ne "
#ifndef BUILD_INFO_HPP
#define BUILD_INFO_HPP

#define BUILD_VERSION          \"$(sed -n 's/.*VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt | tail -n 1)\"
#define BUILD_TYPE             \"Release\"
#define BUILD_DATE             \"$(date +%Y-%m-%d)\"
#define BUILD_TIME             \"$(date +%H:%M:%S)\"
#define BUILD_FLAGS            \"-Wall;-Werror;-Wno-deprecated-declarations;-Os;-fexceptions;-DANDROID_BUILD\"
#define BUILD_CMAKE_VERSION    \"$(if [ -f .cmake_version ]; then cat .cmake_version; else echo unknown; fi)\"
#define BUILD_COMPILER_VERSION \"android-clang-unknown\"
#define COMMIT_ID              \"$(if which git &>/dev/null; then git rev-parse --short HEAD; else echo xxxxxxx; fi)\"

#endif
"
