/*
 * Copyright (C) 2026 Yağız Zengin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PARTITION_MANAGER__PARTITION_MANAGER_HPP
#define PARTITION_MANAGER__PARTITION_MANAGER_HPP

#if __cplusplus < 202002L
#error "Partition Manager is requires C++20 or higher C++ standarts."
#endif

#include <memory>
#include <string>
#include <set>
#include <libhelper/lib.hpp>
#include <libpartition_map/lib.hpp>
#ifdef ERR
#undef ERR
#define ERR PartitionManager::Error()
#endif

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
  bool viewLicense;
  bool forceProcess;
  bool noWorkOnUsed;
};

using Error = Helper::Error;
using FlagsBase = std::shared_ptr<BasicFlags>;
} // namespace PartitionManager

#endif // #ifndef PARTITION_MANAGER__PARTITION_MANAGER_HPP
