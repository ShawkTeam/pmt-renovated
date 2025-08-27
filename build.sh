#!/usr/bin/bash
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

THIS="$(basename $0)"
TARGET_ABI_LIST=("arm64-v8a" "armeabi-v7a")

echo() { command echo "[$THIS]: $@"; }

checks() {
    if [ -z "$ANDROID_NDK" ]; then
        echo "Please set ANDROID_NDK variable as your NDK path."
        exit 1
    fi
    if ! which cmake &>/dev/null; then
        echo "Please verify your CMake installation."
        exit 1
    fi
    if ! which ninja &>/dev/null; then
        echo "Please verify your Ninja installation."
        exit 1
    fi
}

clean() {
    echo "Cleaning workspace."
    for a in ${TARGET_ABI_LIST[@]}; do rm -rf build_$a; done
    rm -rf include/generated \
        srclib/libhelper/tests/dir \
        srclib/libhelper/tests/linkdir \
        srclib/libhelper/tests/file.txt
}

build() {
    set -e
    command echo -e "BUILD INFO:
    ARCHS: ${TARGET_ABI_LIST[@]}
    ANDROID_PLATFORM: $ANDROID_PLATFORM
    ANDROID_TOOLCHAIN_FILE: $ANDROID_NDK/build/cmake/android.toolchain.cmake\n"

    for a in ${TARGET_ABI_LIST[@]}; do
        echo "Configuring for $a..."
        mkdir -p build_$a
        cmake -B build_$a -G Ninja -S . $1 \
            -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
            -DANDROID_ABI=$a \
            -DANDROID_PLATFORM=$ANDROID_PLATFORM \
            -DANDROID_STL=c++_static
    done

    for a in ${TARGET_ABI_LIST[@]}; do
        echo "Building $a artifacts..."
        cmake --build build_$a -j$(($(nproc) - 2))
        echo "$a build complete, artifacts: $PWD/build_$a"
    done
}

if [ $# -eq 0 ]; then
    command echo -e "Usage: $0 build|rebuild|clean [EXTRA_CMAKE_FLAGS]\n  HINT: Export ANDROID_PLATFORM if you set min Android target.\n  HINT: Change TARGET_ABI_LIST array in build.sh if you build other archs."
    exit 1
fi

[ -z $ANDROID_PLATFORM ] && ANDROID_PLATFORM="android-21"
checks

case $1 in
    "build")   build $2;;
    "clean")   clean ;;
    "rebuild") clean; build $2;;
    *)
        command echo "$0: Unknown argument: $1"
        exit 1 ;;
esac
