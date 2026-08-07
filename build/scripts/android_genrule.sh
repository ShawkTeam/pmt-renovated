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
CMAKELISTS_PATH="$2/CMakeLists.txt"
FULL_VERSION=$(sed -n 's/.*VERSION \([0-9.]*\).*/\1/p' "$CMAKELISTS_PATH" | tail -n 1)
MAJOR=$(echo "$FULL_VERSION" | cut -d. -f1)
MINOR=$(echo "$FULL_VERSION" | cut -d. -f2)
PATCH=$(echo "$FULL_VERSION" | cut -d. -f3)

MAJOR=${MAJOR:-0}
MINOR=${MINOR:-0}
PATCH=${PATCH:-0}

if command -v git &>/dev/null && git rev-parse --git-dir &>/dev/null; then
    COMMIT_ID=$(git rev-parse --short HEAD)
else
    COMMIT_ID="xxxxxxx"
fi

cat <<EOF
#ifndef BUILD_INFO_HPP
#define BUILD_INFO_HPP

#define BUILD_VERSION          "${FULL_VERSION}"
#define BUILD_VERSION_MAJOR    ${MAJOR}
#define BUILD_VERSION_MINOR    ${MINOR}
#define BUILD_VERSION_PATCH    ${PATCH}
#define BUILD_TYPE             "Release"
#define BUILD_DATE             "$(date +%Y-%m-%d)"
#define BUILD_TIME             "$(date +%H:%M:%S)"
#define COMMIT_ID              "${COMMIT_ID}"

#define BUILD_STR_HELPER(x) #x
#define BUILD_STR(x)        BUILD_STR_HELPER(x)

/* Compiler Version Detection */
#if defined(__clang__)
    #define BUILD_COMPILER_VERSION "clang " BUILD_STR(__clang_major__) "." BUILD_STR(__clang_minor__) "." BUILD_STR(__clang_patchlevel__)
#elif defined(__GNUC__)
    #define BUILD_COMPILER_VERSION "gcc " BUILD_STR(__GNUC__) "." BUILD_STR(__GNUC_MINOR__) "." BUILD_STR(__GNUC_PATCHLEVEL__)
#else
    #define BUILD_COMPILER_VERSION "compiler-unknown"
#endif

/* Target Architecture / ABI Detection */
#if defined(__aarch64__)
    #define BUILD_ARCH         "arm64-v8a"
#elif defined(__arm__)
    #define BUILD_ARCH         "armeabi-v7a"
#elif defined(__x86_64__)
    #define BUILD_ARCH         "x86_64"
#elif defined(__i386__)
    #define BUILD_ARCH         "x86"
#else
    #define BUILD_ARCH         "unknown"
#endif

#endif // BUILD_INFO_HPP
EOF
