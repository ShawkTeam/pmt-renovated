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

THIS="$(basename $0)"
WORK_DIR="$(pwd)"

if [ $# -gt 0 ]; then
  while [ $# -gt 0 ]; do
    case "$1" in
        --working-directory)
            [ -z "$2" ] && { echo "$THIS: --working-directory requires a value"; exit 1; }
            [ -e "$2" ] || { echo "$THIS: $2: No such file or directory"; exit 1; }
            [ -d "$2" ] || { echo "$THIS: $2: Is not directory"; exit 1; }
            WORK_DIR="$2"
            shift 2
            ;;
        --working-directory=*)
            wd="${1#--working-directory=}"
            [ -z "$wd" ] && { echo "$THIS: --working-directory requires a value"; exit 1; }
            [ -e "$wd" ] || { echo "$THIS: $wd: No such file or directory"; exit 1; }
            [ -d "$wd" ] || { echo "$THIS: $wd: Is not directory"; exit 1; }
            WORK_DIR="$wd"
            shift
            ;;
        --ignore-file)
            [ -z "$2" ] && { echo "$THIS: --ignore-file requires a value"; exit 1; }
            IGNORE_FILE="$2"
            shift 2
            ;;
        --ignore-file=*)
            igf="${1#--ignore-file=}"
            [ -z "$igf" ] && { echo "$THIS: --ignore-file requires a value"; exit 1; }
            IGNORE_FILE="$igf"
            shift
            ;;
        *)
            echo "$THIS: Unknow argument: $1"
            exit 1
            ;;
    esac
  done
fi

[ -z "$IGNORE_FILE" ] && IGNORE_FILE="${WORK_DIR}/.clang-format-ignore"

if [ ! -f "$IGNORE_FILE" ]; then
  echo "Cannot find $IGNORE_FILE file!"
  exit 1;
fi

MAPFILE=()
while IFS= read -r line; do
    [[ "$line" =~ ^#.*$ || -z "$line" ]] && continue
    MAPFILE+=("$line")
done < "$IGNORE_FILE"

FIND_ARGS=()
FIRST=1
for item in "${MAPFILE[@]}"; do
    if [[ "$item" == */ ]]; then
        dir_path="./${item%/}"
        if [ $FIRST -eq 0 ]; then FIND_ARGS+=("-o"); fi
        FIND_ARGS+=("-path" "$dir_path")
        FIRST=0
    fi
done

if [ ${#FIND_ARGS[@]} -eq 0 ]; then
    FIND_ARGS+=("-false")
fi

EXCLUDE_ARGS=()
for item in "${MAPFILE[@]}"; do
    if [[ "$item" == */ ]]; then
        dir_path="${WORK_DIR}/${item%/}"
        if [ $FIRST -eq 0 ]; then FIND_ARGS+=("-o"); fi
        FIND_ARGS+=("-path" "$dir_path" "-o" "-path" "${dir_path}/*")
        FIRST=0
    fi
done

find ${WORK_DIR} \( "${FIND_ARGS[@]}" \) -prune -o \
     -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
     "${EXCLUDE_ARGS[@]}" -print \
     -exec clang-format -i {} +
