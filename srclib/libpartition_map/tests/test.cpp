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

#include <iostream>
#include <libpartition_map/lib.hpp>
#include <unistd.h>

int main() {
  if (getuid() != 0) return 2;

  try {
    PartitionMap::BuildMap MyMap;
    if (!MyMap) {
      MyMap.readDirectory("/dev/block/by-name");
      if (!MyMap) throw PartitionMap::Error("Cannot generate object!");
    }

    const auto map = MyMap.getAll();
    if (map.empty()) throw PartitionMap::Error("getAll() empty");
    for (const auto &[name, props] : map) {
      std::cout << "Partition: " << name << ", size: " << props.size
                << ", logical: " << props.isLogical << std::endl;
    }

    const auto boot = MyMap.get("boot");
    if (!boot) throw PartitionMap::Error("get(\"boot\") returned nullopt");
    std::cout << "Name: boot" << ", size: " << boot->first
              << ", logical: " << boot->second << std::endl;

    const auto logicals = MyMap.getLogicalPartitionList();
    if (!logicals)
      throw PartitionMap::Error("getLogicalPartitionList() returned nullopt");
    std::cout << "Logical partitions: " << std::endl;
    for (const auto &name : *logicals)
      std::cout << "   - " << name << std::endl;

    const auto physicals = MyMap.getPhysicalPartitionList();
    if (!physicals)
      throw PartitionMap::Error("getPhysicalPartitionList() returned nullopt");
    std::cout << "Physical partitions: " << std::endl;
    for (const auto &name : *physicals)
      std::cout << "   - " << name << std::endl;

    std::cout << "Boot: " << MyMap.getRealLinkPathOf("boot") << std::endl;
    std::cout << "Boot (realpath): " << MyMap.getRealPathOf("boot")
              << std::endl;
    std::cout << "Search dir: " << MyMap.getCurrentWorkDir() << std::endl;
    std::cout << "Has partition cache? = " << MyMap.hasPartition("cache")
              << std::endl;
    std::cout << "system partition is logical? = " << MyMap.isLogical("system")
              << std::endl;
    std::cout << "Size of system partition: " << MyMap.sizeOf("system")
              << std::endl;

    MyMap.clear();
    if (!MyMap.empty()) throw PartitionMap::Error("map cleaned but check fail");

    MyMap.readDirectory("/dev/block/by-name");
    PartitionMap::BuildMap MyMap2;

    if (MyMap == MyMap2) std::cout << "map1 = map2" << std::endl;
    if (MyMap != MyMap2) std::cout << "map1 != map2" << std::endl;

    std::cout << PartitionMap::getLibVersion() << std::endl;
  } catch (PartitionMap::Error &error) {
    std::cerr << error.what() << std::endl;
    return 1;
  }

  return 0;
}
