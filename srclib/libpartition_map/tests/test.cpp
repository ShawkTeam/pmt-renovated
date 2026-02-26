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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <libhelper/error.hpp>
#include <libhelper/functions.hpp>
#include <libpartition_map/lib.hpp>

int main() {
  if (!Helper::hasSuperUser()) return 2; // Check root access.

  try {
    PartitionMap::Builder partitions;

    if (!partitions.valid() && !partitions) throw Helper::Error("Tables not valid (UNEXPECTED?)");

    if (partitions.empty()) throw Helper::Error("Class is empty (UNEXPECTED)");

    PartitionMap::Builder partitions2 = partitions;
    if (partitions2 == partitions)
      std::cout << "partitions2 and partitions is same" << std::endl;
    else
      throw Helper::Error("partitions2 and partitions are different (UNEXPECTED)");

    partitions2.clear();

    std::string tableName = partitions.hasTable("mmcblk0") ? "mmcblk0" : "sda";
    if (!partitions.hasTable(tableName)) throw Helper::Error("Can't find mmcblk0 or sda (UNEXPECTED?)");
    auto data = partitions.hasTable("mmcblk0") ? partitions["mmcblk0"] : partitions["sda"];
    if (data->GetNumParts() == 0) throw Helper::Error("Can't get total partition number of mmcblk0 or sda (UNEXPECTED?)");

    if (auto part = partitions(tableName, 0); !part->IsUsed())
      std::cerr << "WARNING: (GPTPart part = partitions[0]) check failed "
                   "(part.IsUsed() returned false)"
                << std::endl;

    auto partition_list = partitions.partitions();
    std::cout << "Listing partitions (data is getted from getPartitions()):" << std::endl;
    for (auto &part : partition_list)
      std::cout << std::setw(16) << part->name() << std::endl;

    auto partitions_of_disk = partitions.partitionsByTable(tableName);
    std::cout << "Listing partitions of table (data is getted from "
                 "getPartitionsByTable()):"
              << std::endl;
    for (auto &part : partitions_of_disk)
      std::cout << std::setw(16) << part->name() << std::endl;

    auto logical_partitions = partitions.logicalPartitions();
    std::cout << "Listing logical partitions:" << std::endl;
    for (auto &part : logical_partitions)
      std::cout << std::setw(10) << part->name() << std::endl;

    auto readed_gpt_data_collection = partitions.allGPTData();
    std::cout << "Listing readed gpt data paths:" << std::endl;
    std::ranges::for_each(readed_gpt_data_collection, [](const auto &info) { std::cout << " " << info.first; });
    std::cout << std::endl;

    auto data2 = partitions.hasTable("mmcblk0") ? partitions.GPTDataOf("mmcblk0") : partitions.GPTDataOf("sda");
    if (data2->GetNumParts() == 0) throw Helper::Error("Can't get gpt data of mmcblk0 or sda (UNEXPECTED?)");

    if (auto logical_partition_data = partitions.dataOfLogicalPartitions(); logical_partition_data.empty())
      std::cerr << "WARNING: Can't get data of logical partitions" << std::endl;

    if (auto partition_data = partitions.dataOfPartitions(); partition_data.empty())
      std::cerr << "WARNING: Can't get data of partitions" << std::endl;

    if (auto partition_data2 = partitions.dataOfPartitionsByTable("mmcblk0"); partition_data2.empty()) {
      partition_data2 = partitions.dataOfPartitionsByTable("sda");
      if (partition_data2.empty())
        std::cerr << "WARNING: Can't get data of partitions (with "
                     "getDataOfPartititonsByDisk())"
                  << std::endl;
    }

    std::cout << "Boot partition is exists?: " << std::boolalpha << partitions.hasPartition("boot") << std::endl;
    std::cout << "System (logical) partition is exists?: " << std::boolalpha << partitions.hasLogicalPartition("system") << std::endl;
    std::cout << "mmcblk0, sda tables is exists?: " << std::boolalpha << partitions.hasTable("mmcblk0") << ", "
              << partitions.hasTable("sda") << std::endl;
    std::cout << "Has super partition?: " << std::boolalpha << partitions.isHasSuperPartition() << std::endl;
    std::cout << "System partition is logical?: " << std::boolalpha << partitions.isLogical("system") << std::endl;
    std::cout << "Disk names are empty?: " << std::boolalpha << partitions.tableNamesEmpty() << std::endl << std::endl;

    std::function logicalPartTest = [] FOREACH_PARTITIONS_LAMBDA_PARAMETERS -> bool {
      std::cout << std::quoted(partition.name()) << ":" << std::endl;
      std::cout << "    Size: " << partition.size() << std::endl;
      std::cout << "    Path and absolute path: " << partition.path() << ", " << partition.absolutePath() << std::endl;
      return true;
    };
    std::function partitionTest = [] FOREACH_PARTITIONS_LAMBDA_PARAMETERS -> bool {
      std::cout << std::quoted(partition.name()) << ":" << std::endl;
      std::cout << "    Size: " << partition.formattedSizeString(PartitionMap::MiB) << std::endl;
      std::cout << "    Path and by-name path: " << partition.path() << ", " << partition.pathByName() << std::endl;
      std::cout << "    Index: " << partition.index() << std::endl;
      std::cout << "    Start and end bytes: " << partition.start() << ", " << partition.end() << std::endl;
      std::cout << "    GUID: " << partition.GUIDAsString() << std::endl;
      std::cout << "    Is super partition or super-like partition?: " << std::boolalpha << partition.isSuperPartition() << std::endl;
      return true;
    };
    std::function gptDataTest = [] FOREACH_GPT_DATA_LAMBDA_PARAMETERS -> bool {
      std::cout << std::quoted(path.string()) << ":" << std::endl;
      std::cout << "    Max partition count: " << gptData->GetNumParts() << std::endl;
      std::cout << "    Total partition count: " << gptData->CountParts() << std::endl;
      std::cout << "    Type (number): " << gptData->GetState() << std::endl;
      std::cout << "    Block size: " << gptData->GetBlockSize() << std::endl;
      return true;
    };
    partitions.foreachLogicalPartitions(logicalPartTest);
    partitions.foreachPartitions(partitionTest);
    partitions.foreachGptData(gptDataTest);

    partitions.reScan();
    if (partitions.empty()) throw Helper::Error("Class was empty after reScan() (UNEXPECTED)");

    partitions.clear();
    if (!partitions.empty()) throw Helper::Error("Class was not empty after clear() (UNEXPECTED)");
  } catch (std::exception &error) {
    std::cerr << error.what() << std::endl;
    return 1;
  }

  return 0;
}
