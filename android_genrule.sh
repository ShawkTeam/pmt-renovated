#!/usr/bin/bash
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
