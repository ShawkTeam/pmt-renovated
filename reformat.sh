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

IGNORE_FILE=".clang-format-ignore"

if [ ! -f $IGNORE_FILE ]; then
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
    if [[ "$item" != */ ]]; then
        EXCLUDE_ARGS+=("!" "-path" "./$item")
    fi
done

find . \( "${FIND_ARGS[@]}" \) -prune -o \
     -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
     "${EXCLUDE_ARGS[@]}" -print \
     -exec clang-format -i {} +
