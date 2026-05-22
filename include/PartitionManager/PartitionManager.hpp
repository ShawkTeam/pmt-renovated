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
 * @file PartitionManager.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Partition Manager header file.
 */

#ifndef PARTITION_MANAGER__PARTITION_MANAGER_HPP
#define PARTITION_MANAGER__PARTITION_MANAGER_HPP

#if __cplusplus < 202002L
#error "Partition Manager is requires C++20 or higher C++ standarts."
#endif

#include <memory>
#include <string>
#include <format>
#include <libhelper/lib.hpp>
#include <libpartition_map/lib.hpp>

/**
 * @namespace PartitionManager
 * @brief Main namespace of pmt.
 */
namespace PartitionManager {
/// @brief Returns the version of pmt.
std::string getAppVersion();

/// @brief Basic flag structure of pmt.
class BasicFlags {
public:
  BasicFlags();

  std::unique_ptr<PartitionMap::Builder> partitionTables; ///< Partition tables.
  std::string logFile;                                    ///< Log file path.

  bool onLogical;    ///< Only process logical partitions.
  bool quietProcess; ///< Turn on/off quiet processing.
  bool verboseMode;  ///< Turn on/off verbose processing.
  bool viewVersion;  ///< Print version and exit.
  bool viewLicense;  ///< View license and exit.
  bool forceProcess; ///< Enable force processes.
  bool noWorkOnUsed; ///< Don't work on used partitions.
};

using Error = Helper::Error;
} // namespace PartitionManager

#endif // #ifndef PARTITION_MANAGER__PARTITION_MANAGER_HPP
