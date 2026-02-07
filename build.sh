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
}

clean() {
    echo "Cleaning workspace."
    for a in "${TARGET_ABI_LIST[@]}"; do rm -rf "build_$a" && rm -rf "build_$a-builtin"; done
    rm -rf include/generated \
        srclib/libhelper/tests/dir \
        srclib/libhelper/tests/linkdir \
        srclib/libhelper/tests/file.txt
}

build() {
    set -e
    command echo -e "BUILD INFO:
    ARCHS: ${TARGET_ABI_LIST[*]}
    ANDROID_PLATFORM: $ANDROID_PLATFORM
    ANDROID_TOOLCHAIN_FILE: $ANDROID_NDK/build/cmake/android.toolchain.cmake\n"

    for a in "${TARGET_ABI_LIST[@]}"; do
        echo "Configuring for $a..."
        mkdir -p "build_$a" "build_$a-builtin"
        cmake -B "build_$a" -G Ninja -S . "$@" \
            -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
            -DANDROID_ABI="$a" \
            -DANDROID_PLATFORM="$ANDROID_PLATFORM" \
            -DANDROID_STL=c++_static
        cmake -B "build_$a-builtin" -G Ninja -S . "$@" \
            -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
            -DANDROID_ABI="$a" \
            -DANDROID_PLATFORM="$ANDROID_PLATFORM" \
            -DANDROID_STL=c++_static \
            -DBUILTIN_PLUGINS=ON
    done

    for a in "${TARGET_ABI_LIST[@]}"; do
        echo "Building $a artifacts... Using $(($(nproc) - 2)) thread."
        cmake --build "build_$a" -j$(($(nproc) - 2))
        echo "$a build complete, artifacts: $PWD/build_$a"

        echo "Building $a-builtin artifacts... Using $(($(nproc) - 2)) thread."
        cmake --build "build_$a-builtin" -j$(($(nproc) - 2))
        echo "$a-builtin build complete, artifacts: $PWD/build_$a-builtin"
    done
}

if [ $# -eq 0 ]; then
    command echo -e "Usage: $0 build|rebuild|clean [EXTRA_CMAKE_FLAGS]\n  HINT: Export ANDROID_PLATFORM if you set min Android target.\n  HINT: Change TARGET_ABI_LIST array in build.sh if you build other archs."
    exit 1
fi

[ -z "$ANDROID_PLATFORM" ] && ANDROID_PLATFORM="android-24"
checks

case $1 in
    "build")   shift; build "$@";;
    "clean")   clean ;;
    "rebuild") clean; shift; build "$@";;
    *)
        command echo "$0: Unknown argument: $1"
        exit 1 ;;
esac
