/*
   Copyright 2026 Yağız Zengin

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

#ifndef PARTITION_MANAGER__PARTITION_MANAGER_HPP
#define PARTITION_MANAGER__PARTITION_MANAGER_HPP

#include <memory>
#include <string>
#include <set>
#include <libpartition_map/lib.hpp>

class Out {
public:
  static __printflike(1, 2) void print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
  }

  static __printflike(1, 2) void println(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    print("\n");
    va_end(args);
  }
};

namespace PartitionManager {
std::string getAppVersion(); // Not Android app version (an Android app is
                             // planned!), tells pmt version.

class BasicFlags {
public:
  BasicFlags();

  std::unique_ptr<PartitionMap::Builder> partitionTables;
  std::string logFile;
  std::set<std::string> extraTablePaths;

  bool onLogical;
  bool quietProcess;
  bool verboseMode;
  bool viewVersion;
  bool forceProcess;
  bool noWorkOnUsed;
};

using Error = Helper::Error;
using FlagsBase = std::shared_ptr<BasicFlags>;
} // namespace PartitionManager

#endif // #ifndef PARTITION_MANAGER__PARTITION_MANAGER_HPP
