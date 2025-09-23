/*
   Copyright 2025 Yağız Zengin

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <PartitionManager/PartitionManager.hpp>
#include <algorithm>
#include <fcntl.h>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace PartitionManager {
std::vector<std::string> splitIfHasDelim(const std::string &s, const char delim,
                                         const bool checkForBadUsage) {
  if (s.find(delim) == std::string::npos) return {};
  auto vec = CLI::detail::split(s, delim);

  if (checkForBadUsage) {
    std::unordered_set<std::string> set;
    for (const auto &str : vec) {
      if (set.find(str) != set.end())
        throw CLI::ValidationError("Duplicate element in your inputs!");
      set.insert(str);
    }
  }

  return vec;
}

void setupBufferSize(uint64_t &size, const std::string &entry) {
  if (PART_MAP.hasPartition(entry) && PART_MAP.sizeOf(entry) % size != 0) {
    println("%sWARNING%s: Specified buffer size is invalid for %s! Using "
            "different buffer size for %s.",
            YELLOW, STYLE_RESET, entry.data(), entry.data());
    size = PART_MAP.sizeOf(entry) % 4096 == 0 ? 4096 : 1;
  } else if (Helper::fileIsExists(entry)) {
    if (Helper::fileSize(entry) % size != 0) {
      println("%sWARNING%s: Specified buffer size is invalid for %s! using "
              "different buffer size for %s.",
              YELLOW, STYLE_RESET, entry.data(), entry.data());
      size = Helper::fileSize(entry) % 4096 == 0 ? 4096 : 1;
    }
  }
}

void processCommandLine(std::vector<std::string> &vec1,
                        std::vector<std::string> &vec2, const std::string &s1,
                        const std::string &s2, const char delim,
                        const bool checkForBadUsage) {
  vec1 = splitIfHasDelim(s1, delim, checkForBadUsage);
  vec2 = splitIfHasDelim(s2, delim, checkForBadUsage);

  if (vec1.empty() && !s1.empty()) vec1.push_back(s1);
  if (vec2.empty() && !s2.empty()) vec2.push_back(s2);
}
} // namespace PartitionManager
