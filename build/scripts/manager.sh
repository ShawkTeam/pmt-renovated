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
DEFAULT_RELEASE="20260207"
RELEASE="${DEFAULT_RELEASE}"
VARIANT=""

echo() { command echo "[$THIS]: $*"; }
error() { echo "$*"; exit 1; }

checks() {
	curl "https://github.com" &>/dev/null || error "No internet connection!"

	echo "Updating repositories and checking required packages..."
	pkg update &>/dev/null
	[ ! -f "$PREFIX/bin/unzip" ] && pkg install -y unzip
	[ ! -f "$PREFIX/bin/wget" ] && pkg install -y wget
	[ ! -f "$PREFIX/bin/jq" ] && pkg install -y jq
}

get_latest_release() {
	echo "Fetching latest release information..."
	LATEST_RELEASE=$(curl -s "https://api.github.com/repos/ShawkTeam/pmt-renovated/releases/latest" | jq -r '.tag_name' 2>/dev/null)
	if [ -z "$LATEST_RELEASE" ] || [ "$LATEST_RELEASE" = "null" ]; then
		echo "Warning: Could not fetch latest release, using default: $DEFAULT_RELEASE"
		RELEASE="$DEFAULT_RELEASE"
	else
		RELEASE="$LATEST_RELEASE"
		echo "Latest release found: $RELEASE"
	fi
	
	# Ensure RELEASE is set globally
	export RELEASE
}

select_variant() {
	export LINK ARCH VARIANT

	if getprop ro.product.cpu.abi | grep "arm64-v8a" &>/dev/null; then ARCH="arm64-v8a"
	else ARCH="armeabi-v7a"
	fi

	LINK="https://github.com/ShawkTeam/pmt-renovated/releases/download/${RELEASE}/pmt-${VARIANT}${ARCH}-builtin.zip"
}

download() {
	echo "Downloading pmt-${VARIANT}${ARCH}-builtin.zip (${RELEASE})"
	if ! wget -O "$PREFIX/tmp/pmt.zip" "${LINK}" &>/dev/null; then
		rm -f "$PREFIX/tmp"/*.zip
		error "Download failed! LINK=${LINK}"
	fi

	echo "Extracting..."
	unzip -o -d "$PREFIX/tmp" "$PREFIX/tmp/pmt.zip" &>/dev/null || error "Extraction failed!"
}

setup() {
	[ -f "$PREFIX/tmp/pmt_static" ] && mv "$PREFIX/tmp/pmt_static" "$PREFIX/tmp/pmt"
	set -e
	install -t "$PREFIX/bin" "$PREFIX/tmp/pmt"
	if find "$PREFIX/tmp" -name "lib*.so" &>/dev/null; then
		find "$PREFIX/tmp" -name "lib*.so" -exec install -t "$PREFIX/lib" {} \;
		echo "Libraries installed to $PREFIX/lib"
	fi
	echo "Installed successfully. Try running 'pmt' command."
}

uninstall() {
	rm -f "$PREFIX/bin/pmt" "$PREFIX"/lib/libhelper* "$PREFIX"/lib/libpartition_map* &>/dev/null
}

is_installed() {
	/system/bin/which pmt &>/dev/null && error "PMT is already installed."
}

cleanup() {
	rm -f "$PREFIX/tmp"/pmt* "$PREFIX/tmp"/lib* "$PREFIX/tmp"/*.zip &>/dev/null
}

show_help() {
	cat << EOF
Usage: $0 [COMMAND] [OPTIONS]

COMMANDS:
    install [TAG]     Install PMT. If TAG is not specified, uses the latest release.
    uninstall          Remove PMT installation.
    reinstall [TAG]    Reinstall PMT. If TAG is not specified, uses the latest release.
    help               Show this help message.

EXAMPLES:
    $0 install                    # Install latest release
    $0 install v20260207          # Install specific tag/release
    $0 reinstall                  # Reinstall latest release
    $0 reinstall v20260115        # Reinstall specific tag/release

VARIANTS:
    The script automatically detects your architecture (arm64-v8a or armeabi-v7a).
EOF
}

# Parse command line arguments
COMMAND="$1"
TAG_ARG="$2"

if [ -z "$COMMAND" ]; then
    show_help
    exit 1
fi

basename -a "${PREFIX}"/bin/* | grep "termux" &>/dev/null \
  || error "This script only works for Termux!"

# Handle help command
if [ "$COMMAND" = "help" ] || [ "$COMMAND" = "-h" ] || [ "$COMMAND" = "--help" ]; then
    show_help
    exit 0
fi

# Set release version based on tag argument
if [ -n "$TAG_ARG" ]; then
    RELEASE="$TAG_ARG"
    echo "Using specified tag: $RELEASE"
else
    get_latest_release
fi

# Validate release format (should be like v20260207 or just 20260207)
echo "Debug: Validating release format for '$RELEASE'"
if echo "$RELEASE" | grep -E '^v?[0-9]{8}$' &>/dev/null; then
    echo "Debug: Release tag format is valid: $RELEASE"
else
    echo "Warning: Release tag format '$RELEASE' is not valid. Expected format: vYYYYMMDD or YYYYMMDD"
    echo "Debug: This may cause download issues. Using current value anyway."
fi

case $COMMAND in
    "install")
    	is_installed
    	checks
    	select_variant
		download
		setup
		cleanup
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
      cleanup
    ;;
    *)
      echo "Error: Unknown command: $COMMAND"
      echo
      show_help
      exit 1 ;;
esac
