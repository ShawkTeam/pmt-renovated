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
RELEASE="20250811"

echo() { command echo "[$THIS]: $@"; }

checks()
{
	if ! curl "https://github.com" &>/dev/null; then
		echo "No internet connection!"
		exit 1
	fi

	echo "Updating repositories and checking required packages..."
	pkg update &>/dev/null
	[ ! -f $PREFIX/bin/unzip ] && pkg install -y unzip
    [ ! -f $PREFIX/bin/wget ] && pkg install -y wget
}

select_variant()
{
	LINK=""; ARCH=""; VARIANT=""

	if getprop ro.product.cpu.abi | grep "arm64-v8a" &>/dev/null; then ARCH="arm64-v8a";
	else ARCH="armeabi-v7a"
	fi
    [ -n $1 ] && VARIANT="static-"

    LINK="https://github.com/ShawkTeam/pmt-renovated/releases/download/${RELEASE}/pmt-${VARIANT}${ARCH}.zip"
}

download()
{
	echo "Downloading pmt-${VARIANT}${ARCH}.zip (${RELEASE})"
	if ! wget -O $PREFIX/tmp/pmt.zip "${LINK}" &>/dev/null; then
		echo "Download failed! LINK=${LINK}"
		rm $PREFIX/tmp/*.zip
		exit 1
	fi

	echo "Extracting..."
	if ! unzip -f -d $PREFIX/tmp $PREFIX/tmp/pmt.zip &>/dev/null; then
		echo "Extraction failed!"
		exit 1
	fi
}

setup()
{
	[ -f $PREFIX/tmp/pmt_static ] && mv $PREFIX/tmp/pmt_static $PREFIX/tmp/pmt
	set -e
	install -t $PREFIX/bin $PREFIX/tmp/pmt
	if [ -f $PREFIX/tmp/libhelper.so ]; then
		install -t $PREFIX/lib $(find $PREFIX/tmp -name "lib*.so")
		install -t $PREFIX/lib $(find $PREFIX/tmp -name "lib*.a")
	fi
	echo "Installed successfully. Try running 'pmt' command."
}

uninstall()
{
	rm -f $PREFIX/bin/pmt $PREFIX/lib/libhelper* $PREFIX/lib/libpartition_map* &>/dev/null
}

is_installed()
{
	if /system/bin/which pmt &>/dev/null; then
		echo "PMT is already installed."
		exit 1
	fi
}

cleanup()
{
	rm -f $PREFIX/tmp/pmt* $PREFIX/tmp/lib* $PREFIX/tmp/*.zip &>/dev/null
}

if [ $# -eq 0 ]; then
    command echo "Usage: $0 install|uninstall [--static]"
    exit 1
fi

if ! basename -a ${PREFIX}/bin/* | grep "termux" &>/dev/null; then
	echo "This script only for termux!"
	exit 1
fi

case $1 in
    "install")
    	is_installed
    	checks
    	select_variant $([ "$2" == "--static" ] && echo static)
		download
		setup
    ;;
    "uninstall")
    	uninstall && echo "Uninstalled successfully."
    ;;
    "reinstall")
    	uninstall
    	checks
        select_variant $([ "$2" == "--static" ] && echo static)
        download
        setup
    ;;
    *)
        command echo "$0: Unknown argument: $1"
        exit 1 ;;
esac
