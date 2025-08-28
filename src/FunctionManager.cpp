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
  if (Variables->PartMap->hasPartition(entry) &&
      Variables->PartMap->sizeOf(entry) % size != 0) {
    println("%sWARNING%s: Specified buffer size is invalid for %s! Using "
            "different buffer size for %s.",
            YELLOW, STYLE_RESET, entry.data(), entry.data());
    size = Variables->PartMap->sizeOf(entry) % 4096 == 0 ? 4096 : 1;
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

void basic_function_manager::registerFunction(
    std::unique_ptr<basic_function> _func, CLI::App &_app) {
  LOGN(PMTF, INFO) << "registering function: " << _func->name() << std::endl;
  for (const auto &f : _functions) {
    if (std::string(_func->name()) == std::string(f->name())) {
      LOGN(PMTF, INFO) << "Function is already registered: " << _func->name()
                       << ". Skipping." << std::endl;
      return;
    }
  }
  if (!_func->init(_app))
    throw Error("Cannot init function: %s", _func->name());
  _functions.push_back(std::move(_func));
  LOGN(PMTF, INFO) << _functions.back()->name() << " successfully registered."
                   << std::endl;
}

bool basic_function_manager::isUsed(const std::string &name) const {
  if (_functions.empty()) return false;
  for (const auto &func : _functions) {
    if (func->name() == name) return func->isUsed();
  }
  return false;
}

bool basic_function_manager::handleAll() const {
  LOGN(PMTF, INFO) << "running caught function commands in command-line."
                   << std::endl;
  for (const auto &func : _functions) {
    if (func->isUsed()) {
      LOGN(PMTF, INFO) << func->name()
                       << " is calling because used in command-line."
                       << std::endl;
      return func->run();
    }
  }

  LOGN(PMTF, INFO) << "not found any used function from command-line."
                   << std::endl;
  println("Target progress is not specified. Specify a progress.");
  return false;
}
} // namespace PartitionManager
