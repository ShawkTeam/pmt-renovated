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
RELEASE="20260207"

echo() { command echo "[$THIS]: $*"; }
error() { echo "$*"; exit 1; }

checks() {
	curl "https://github.com" &>/dev/null || error "No internet connection!"

	echo "Updating repositories and checking required packages..."
	pkg update &>/dev/null
	[ ! -f "$PREFIX/bin/unzip" ] && pkg install -y unzip
  [ ! -f "$PREFIX/bin/wget" ] && pkg install -y wget
}

select_variant() {
	local LINK ARCH VARIANT

	if getprop ro.product.cpu.abi | grep "arm64-v8a" &>/dev/null; then ARCH="arm64-v8a"
	else ARCH="armeabi-v7a"
	fi

	LINK="https://github.com/ShawkTeam/pmt-renovated/releases/download/${RELEASE}/pmt-${VARIANT}${ARCH}-builtin.zip"
}

download() {
	echo "Downloading pmt-${VARIANT}${ARCH}.zip (${RELEASE})"
	if ! wget -O "$PREFIX/tmp/pmt.zip" "${LINK}" &>/dev/null; then
		rm "$PREFIX/tmp/*.zip"
		error "Download failed! LINK=${LINK}"
	fi

	echo "Extracting..."
	unzip -o -d "$PREFIX/tmp" "$PREFIX/tmp/pmt.zip" &>/dev/null || error "Extraction failed!"
}

setup() {
	[ -f "$PREFIX/tmp/pmt_static" ] && mv "$PREFIX/tmp/pmt_static" "$PREFIX/tmp/pmt"
	set -e
	install -t "$PREFIX/bin" "$PREFIX/tmp/pmt"
	if [ -f "$PREFIX/tmp/libhelper.so" ]; then
	  find "$PREFIX/tmp" -name "lib*.so" -name "lib*.a" -exec install -t "$PREFIX/lib" {} \;
	fi
	echo "Installed successfully. Try running 'pmt' command."
}

uninstall() {
	rm -f "$PREFIX/bin/pmt" "$PREFIX"/lib/libhelper* "$PREFIX"/lib/libpartition_map* &>/dev/null
}

is_installed() {
	/system/bin/which pmt &>/dev/null || error "PMT is already installed."
}

cleanup() {
	rm -f "$PREFIX"/tmp/pmt* "$PREFIX"/tmp/lib* "$PREFIX"/tmp/*.zip &>/dev/null
}

if [ $# -eq 0 ]; then
    command echo "Usage: $0 install|uninstall"
    exit 1
fi

basename -a "${PREFIX}"/bin/* | grep "termux" &>/dev/null \
  || error "This script only for termux!"

case $1 in
    "install")
    	is_installed
    	checks
    	select_variant
			download
			setup
    ;;
    "uninstall")
    	uninstall && echo "Uninstalled successfully."
    ;;
    "reinstall")
    	uninstall
    	checks
      select_variant
      download
      setup
    ;;
    *)
      command echo "$0: Unknown argument: $1"
      exit 1 ;;
esac
