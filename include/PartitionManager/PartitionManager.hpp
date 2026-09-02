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

#include <memory>
#include <string>
#include <libhelper/lib.hpp>
#include <libpartition_map/lib.hpp>

/**
 * @namespace PartitionManager
 * @brief Main namespace of pmt.
 */
namespace PartitionManager {
/// @brief Returns the version of pmt.
std::string getAppVersion();

enum BasicFlagOptions : uint8_t {
  None = 0,
  OnLogical = 1 << 0,   ///< 0x01, Only process logical partitions.
  Quiet = 1 << 1,       ///< 0x02, Turn on/off quiet processing.
  Verbose = 1 << 2,     ///< 0x04, Turn on/off verbose processing.
  ViewVersion = 1 << 3, ///< 0x08, Print version and exit.
  ViewLicense = 1 << 4, ///< 0x10, View license and exit.
  Force = 1 << 5,       ///< 0x20, Enable force processes.
  NoWorkOnUsed = 1 << 6 ///< 0x40, Don't work on used partitions.
};

/// @brief Basic flag structure of pmt.
class BasicFlags {
public:
  BasicFlags();

  std::string logFile; ///< Log file path.
  std::pair<std::unique_ptr<PartitionMap::PartitionTableData>,
            std::unique_ptr<PartitionMap::DynamicTableData>>
      partitionTables;                           ///< Partition tables.
  Helper::FlagCapsule<BasicFlagOptions> options; ///< Basic options.
};

using Error = Helper::Error;
} // namespace PartitionManager

#endif // #ifndef PARTITION_MANAGER__PARTITION_MANAGER_HPP
