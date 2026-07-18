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

/**
 * @file PartitionManager.cpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Implementation of PartitionManager core functionality.
 *
 * This file contains the implementation of the BasicFlags constructor,
 * initialization function, and version string generation for the
 * Partition Manager Tool.
 */

#include <memory>
#include <PartitionManager/PartitionManager.hpp>
#ifndef ANDROID_BUILD
#include <generated/buildInfo.hpp>
#endif

namespace PartitionManager {

/**
 * @brief Constructor for BasicFlags.
 *
 * Initializes the BasicFlags structure with default values and creates
 * partition table data objects for both classic and dynamic partitions.
 */
BasicFlags::BasicFlags()
    : logFile(Helper::Logger::Properties::FILE), onLogical(false), quietProcess(false), verboseMode(false), viewVersion(false),
      viewLicense(false), forceProcess(false), noWorkOnUsed(false) {
  try {
    partitionTables.first = std::make_unique<PartitionMap::PartitionTableData>();
    partitionTables.second = std::make_unique<PartitionMap::DynamicTableData>();
  } catch (...) {
  }
}

/**
 * @brief Initialization function called at program startup.
 *
 * This function is marked with the constructor attribute and is called
 * automatically when the program starts. It enables logging by default.
 */
__attribute__((constructor)) void init() { Helper::Logger::Properties::setLogging(true); }

/**
 * @brief Get the application version string.
 *
 * @return std::string The version string generated using the MKVERSION macro.
 */
std::string getAppVersion() { MKVERSION("pmt"); }

} // namespace PartitionManager
