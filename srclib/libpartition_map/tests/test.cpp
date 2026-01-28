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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libpartition_map/lib.hpp>
#include <unistd.h>

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

    auto data = partitions.hasDisk("mmcblk0") ? partitions["mmcblk0"] : partitions["sda"];
    if (data->GetNumParts() == 0) throw Helper::Error("Can't find mmcblk0 or sda (UNEXPECTED?)");

    if (GPTPart part = partitions[0]; !part.IsUsed())
      std::cerr << "WARNING: (GPTPart part = partitions[2]) check failed "
                   "(part.IsUsed() returned false)"
                << std::endl;

    auto partition_list = partitions.getPartitions();
    std::cout << "Listing partitions (data is getted from getPartitions()):" << std::endl;
    for (auto &part : partition_list)
      std::cout << std::setw(16) << part.getName() << std::endl;

    auto partitions_of_disk = partitions.getPartitionsByDisk("mmcblk0");
    if (partitions_of_disk.empty()) partitions_of_disk = partitions.getPartitionsByDisk("sda");
    std::cout << "Listing partitions of table (data is getted from "
                 "getPartitionsByDisk()):"
              << std::endl;
    for (auto &part : partitions_of_disk)
      std::cout << std::setw(16) << part.getName() << std::endl;

    auto logical_partitions = partitions.getLogicalPartitions();
    std::cout << "Listing logical partitions:" << std::endl;
    for (auto &part : logical_partitions)
      std::cout << std::setw(10) << part.getName() << std::endl;

    auto readed_gpt_data_collection = partitions.getAllGPTData();
    std::cout << "Listing readed gpt data paths:" << std::endl;
    std::ranges::for_each(readed_gpt_data_collection, [](const auto &info) { std::cout << " " << info.first; });
    std::cout << std::endl;

    auto data2 = partitions.hasDisk("mmcblk0") ? partitions.getGPTDataOf("mmcblk0") : partitions.getGPTDataOf("sda");
    if (data2->GetNumParts() == 0) throw Helper::Error("Can't get gpt data of mmcblk0 or sda (UNEXPECTED?)");

    if (auto logical_partition_data = partitions.getDataOfLogicalPartitions(); logical_partition_data.empty())
      std::cerr << "WARNING: Can't get data of logical partitions" << std::endl;

    if (auto partition_data = partitions.getDataOfPartitions(); partition_data.empty())
      std::cerr << "WARNING: Can't get data of partitions" << std::endl;

    if (auto partition_data2 = partitions.getDataOfPartitionsByDisk("mmcblk0"); partition_data2.empty()) {
      partition_data2 = partitions.getDataOfPartitionsByDisk("sda");
      if (partition_data2.empty())
        std::cerr << "WARNING: Can't get data of partitions (with "
                     "getDataOfPartititonsByDisk())"
                  << std::endl;
    }

    std::cout << "Seek: " << partitions.getSeek() << std::endl;

    std::cout << "Boot partition is exists?: " << std::boolalpha << partitions.hasPartition("boot") << std::endl;
    std::cout << "System (logical) partition is exists?: " << std::boolalpha << partitions.hasLogicalPartition("system")
              << std::endl;
    std::cout << "mmcblk0, sda tables is exists?: " << std::boolalpha << partitions.hasDisk("mmcblk0") << ", "
              << partitions.hasDisk("sda") << std::endl;
    std::cout << "Has super partition?: " << std::boolalpha << partitions.isHasSuperPartition() << std::endl;
    std::cout << "System partition is logical?: " << std::boolalpha << partitions.isLogical("system") << std::endl;
    std::cout << "Disk names are empty?: " << std::boolalpha << partitions.diskNamesEmpty() << std::endl << std::endl;

    std::function logicalPartTest = [] FOREACH_LOGICAL_PARTITIONS_LAMBDA_PARAMETERS -> bool {
      std::cout << std::quoted(lpartition.getName()) << ":" << std::endl;
      std::cout << "    Size: " << lpartition.getSize() << std::endl;
      std::cout << "    Path and absolute path: " << lpartition.getPath() << ", " << lpartition.getAbsolutePath()
                << std::endl;
      return true;
    };
    std::function partitionTest = [] FOREACH_PARTITIONS_LAMBDA_PARAMETERS -> bool {
      std::cout << std::quoted(partition.getName()) << ":" << std::endl;
      std::cout << "    Size: " << partition.getFormattedSizeString(PartitionMap::MiB) << std::endl;
      std::cout << "    Path and by-name path: " << partition.getPath() << ", " << partition.getPathByName()
                << std::endl;
      std::cout << "    Index: " << partition.getIndex() + 1 << std::endl;
      std::cout << "    Start and end bytes: " << partition.getStartByte() << ", " << partition.getEndByte()
                << std::endl;
      std::cout << "    GUID: " << partition.getGUIDAsString() << std::endl;
      std::cout << "    Is super partition or super-like partition?: " << std::boolalpha << partition.isSuperPartition()
                << std::endl;
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
