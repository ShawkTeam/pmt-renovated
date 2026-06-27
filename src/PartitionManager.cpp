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

#include <memory>
#include <PartitionManager/PartitionManager.hpp>
#ifndef ANDROID_BUILD
#include <generated/buildInfo.hpp>
#endif

namespace PartitionManager {

BasicFlags::BasicFlags()
    : logFile(Helper::Logger::Properties::FILE), onLogical(false), quietProcess(false), verboseMode(false), viewVersion(false),
      viewLicense(false), forceProcess(false), noWorkOnUsed(false) {
  try {
    partitionTables.first = std::make_unique<PartitionMap::PartitionTableData>();
    partitionTables.second = std::make_unique<PartitionMap::DynamicTableData>();
  } catch (...) {
  }
}

__attribute__((constructor)) void init() { Helper::Logger::Properties::setLogging(true); }

std::string getAppVersion() { MKVERSION("pmt"); }

} // namespace PartitionManager
