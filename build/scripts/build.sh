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

WORK_DIR="$(pwd)"
THIS="$(basename "$0")"
TARGET_ABI_LIST=("arm64-v8a" "armeabi-v7a")

echo() { command echo "[$THIS]: $*"; }

checks() {
    if [ -z "$ANDROID_NDK" ]; then
        echo "Please set ANDROID_NDK variable as your NDK path."
        exit 1
    fi
    if ! which cmake &>/dev/null || ! which ninja &>/dev/null || ! which python &>/dev/null; then
        echo "Please verify your CMake, Ninja and Python installation."
        exit 1
    fi
    if [ "$(git config core.hooksPath)" != ".githooks" ]; then
        git config core.hooksPath .githooks
        echo "Git hooks configured."
    fi
}

clean() {
    echo "Cleaning workspace."
    for a in "${TARGET_ABI_LIST[@]}"; do rm -rf "build_$a" && rm -rf "build_$a-builtin"; done
    rm -rf "${WORK_DIR}/srclib/libhelper/tests/dir" \
        "${WORK_DIR}/srclib/libhelper/tests/linkdir" \
        "${WORK_DIR}/srclib/libhelper/tests/file.txt"
}

build() {
    set -e
    command echo -e "BUILDING PMT!!! BUILD INFO:
    ARCH(S): ${TARGET_ABI_LIST[*]}
    ANDROID_PLATFORM: $ANDROID_PLATFORM
    ANDROID_TOOLCHAIN_FILE: $ANDROID_NDK/build/cmake/android.toolchain.cmake\n"

    for a in "${TARGET_ABI_LIST[@]}"; do
        echo "Configuring for $a..."
        mkdir -p "${WORK_DIR}/build_$a" "${WORK_DIR}/build_$a-builtin"
        cmake -B "${WORK_DIR}/build_$a" -G Ninja -S "${WORK_DIR}" "$@" \
            -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
            -DANDROID_ABI="$a" \
            -DANDROID_PLATFORM="$ANDROID_PLATFORM" \
            -DANDROID_STL=c++_static
        cmake -B "${WORK_DIR}/build_$a-builtin" -G Ninja -S "${WORK_DIR}" "$@" \
            -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
            -DANDROID_ABI="$a" \
            -DANDROID_PLATFORM="$ANDROID_PLATFORM" \
            -DANDROID_STL=c++_static \
            -DBUILTIN_PLUGINS=ON
    done

    for a in "${TARGET_ABI_LIST[@]}"; do
        echo "Building $a artifacts... Using $(($(nproc) - 2)) thread."
        cmake --build "${WORK_DIR}/build_$a" -j$(($(nproc) - 2))
        echo "$a build complete, artifacts: ${WORK_DIR}/build_$a"
        echo "Building $a-builtin artifacts... Using $(($(nproc) - 2)) thread."
        cmake --build "${WORK_DIR}/build_$a-builtin" -j$(($(nproc) - 2))
        echo "$a-builtin build complete, artifacts: ${WORK_DIR}/build_$a-builtin"
    done
}

parse_args() {
    local -A seen_abis=()
    local custom_abis=()
    local cmake_args=()
    local command=""

    while [ $# -gt 0 ]; do
        case "$1" in
            build|rebuild|clean)
                [ -n "$command" ] && { command echo "$THIS: Multiple commands specified: '$command' and '$1'"; exit 1; }
                command="$1"
                shift
                ;;
            --working-directory)
                [ -z "$2" ] && { command echo "$THIS: --working-directory requires a value"; exit 1; }
                [ -e "$2" ] || { command echo "$THIS: $2: No such file or directory"; exit 1; }
                [ -d "$2" ] || { command echo "$THIS: $2: Is not directory"; exit 1; }
                WORK_DIR="$2"
                shift 2
                ;;
            --working-directory=*)
                local wd="${1#--working-directory=}"
                [ -z "$wd" ] && { command echo "$THIS: --working-directory requires a value"; exit 1; }
                [ -e "$wd" ] || { command echo "$THIS: $wd: No such file or directory"; exit 1; }
                [ -d "$wd" ] || { command echo "$THIS: $wd: Is not directory"; exit 1; }
                WORK_DIR="$wd"
                shift
                ;;
            --arch)
                [ -z "$2" ] && { command echo "$THIS: --arch requires a value"; exit 1; }
                if [ -z "${seen_abis[$2]+x}" ]; then
                    seen_abis["$2"]=1
                    custom_abis+=("$2")
                else
                    command echo "$THIS: Duplicate --arch '$2', skipping."
                fi
                shift 2
                ;;
            --arch=*)
                local abi="${1#--arch=}"
                [ -z "$abi" ] && { command echo "$THIS: --arch= requires a value"; exit 1; }
                if [ -z "${seen_abis[$abi]+x}" ]; then
                    seen_abis["$abi"]=1
                    custom_abis+=("$abi")
                else
                    command echo "$THIS: Duplicate --arch '$abi', skipping."
                fi
                shift
                ;;
            -D*|-C*|-U*|-W*)
                cmake_args+=("$1")
                shift
                ;;
            *)
                command echo "$THIS: Unknown argument: '$1'"
                exit 1
                ;;
        esac
    done

    [ -z "$command" ] && { command echo "$THIS: No command specified. Use build, rebuild or clean."; exit 1; }
    [ ${#custom_abis[@]} -gt 0 ] && TARGET_ABI_LIST=("${custom_abis[@]}")

    PARSED_COMMAND="$command"
    PARSED_CMAKE_ARGS=("${cmake_args[@]}")
}

if [ $# -eq 0 ]; then
    command echo -e "Usage: $0 build|rebuild|clean [--arch ABI]... [EXTRA_CMAKE_FLAGS]
  HINT: Export ANDROID_PLATFORM if you set min Android target.
  HINT: Use --arch to override target ABI list (default: ${TARGET_ABI_LIST[*]})
  Example: $0 build --arch arm64-v8a --working-directory \$PWD/other-pmt-renovated-src
  Example: $0 rebuild --arch armeabi-v7a
  Example: $0 build --arch arm64-v8a --arch armeabi-v7a
  Example: $0 clean --arch armeabi-v7a"
    exit 1
fi

[ -z "$ANDROID_PLATFORM" ] && ANDROID_PLATFORM="android-30"
parse_args "$@"
checks

case "$PARSED_COMMAND" in
    "build")   build "${PARSED_CMAKE_ARGS[@]}" ;;
    "clean")   clean ;;
    "rebuild") clean; build "${PARSED_CMAKE_ARGS[@]}" ;;
esac
