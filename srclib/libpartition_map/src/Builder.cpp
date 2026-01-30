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
#include <iostream>
#include <libpartition_map/lib.hpp>
#include <libpartition_map/redefine_logging_macros.hpp>
#include <ranges>
#include <utility>

namespace PartitionMap {

void Builder::scan() {
  if (tableNames.empty()) throw Error("Empty disk path.");
  LOGI << "Cleaning current data and scanning partitions..." << std::endl;
  partitions.clear();
  gptDataCollection.clear();

  for (const auto &name : tableNames) {
    std::filesystem::path p("/dev/block");
    p /= name; // Append device.

    LOGI << "Silencing stdout..." << std::endl << std::flush;
    Helper::Silencer silencer;
    if (auto gpt = std::make_shared<GPTData>(); gpt->LoadPartitions(p)) {
      gptDataCollection[p] = std::move(gpt); // Add to GPT data list.

      auto &storedGpt = *gptDataCollection[p];
      for (uint32_t i = 0; i < storedGpt.GetNumParts(); ++i) {
        if (GPTPart part = storedGpt[i]; part.IsUsed()) {
          partitions.push_back(Partition_t({part, i, p})); // Add to partition list.
          LOGI << "Registered partition: " << part.GetDescription() << std::endl;
        }
      }
    }
  }

  LOGI << "Scan complete!" << std::endl;
  LOGI << "Sorting partitions by name." << std::endl;
  std::ranges::sort(partitions, [](Partition_t a, Partition_t b) { return a.getName() < b.getName(); });
}

void Builder::scanLogicalPartitions() {
  if (!Helper::directoryIsExists("/dev/block/mapper")) return;
  LOGI << "Scanning logical partitions..." << std::endl;

  LOGI << "Reading " << std::quoted("/dev/block/mapper") << " and sorting by name." << std::endl;
  std::vector<std::filesystem::directory_entry> entries{std::filesystem::directory_iterator("/dev/block/mapper"),
                                                        std::filesystem::directory_iterator()};
  std::ranges::sort(entries, [](const auto &a, const auto &b) { return a.path().filename() < b.path().filename(); });

  for (const auto &entry : entries) {
    std::filesystem::path final =
        std::filesystem::is_symlink(entry.path()) ? std::filesystem::read_symlink(entry.path()) : entry.path();
    if (std::filesystem::is_block_file(final)) {
      partitions.emplace_back(entry.path());
      LOGI << "Registered logical partition: " << entry.path().filename() << std::endl;
    }
  }

  LOGI << "Scan complete!" << std::endl;
  LOGI << "Removing non-partition contents from data." << std::endl;
  std::erase_if(partitions, [](Partition_t &part) -> bool {
    if (part.getName().find("com.") != std::string::npos ||
           part.getName() == "userdata") { // Erase non-partition contents (like com.android.adbd) and userdata.
      LOGI << "Removed: " << part.getName() << std::endl;
      return true;
    }
    return false;
  });
}

void Builder::findTablePaths() {
  LOGI << "Finding partition tables in " << std::quoted("/dev/block") << std::endl;
  try {
    std::vector<std::filesystem::directory_entry> entries{std::filesystem::directory_iterator("/dev/block"),
                                                          std::filesystem::directory_iterator()};
    std::ranges::sort(entries, [](const auto &a, const auto &b) { return a.path().filename() < b.path().filename(); });

    for (const auto &entry : entries) {
      if (std::filesystem::is_block_file(entry.status()) && Extra::isReallyTable(entry.path().filename())) {
        LOGI << "Found partition table: " << entry.path() << std::endl;
        tableNames.insert(entry.path().filename());
      }
    }
  } catch (const std::filesystem::filesystem_error &e) {
    throw Error("%s", e.what());
  }

  LOGI << "Find complete!" << std::endl;
  if (tableNames.size() > 1) isUFS = true;
  if (tableNames.empty()) throw Error("Can't find any disk or partition table in \"/dev/block\"");
  seek = *tableNames.begin();
}

std::vector<Partition_t *> Builder::getPartitions() {
  LOGI << "Providing references of all partitions." << std::endl;
  std::vector<Partition_t *> parts;
  for (auto &part : partitions) {
    if (!part.isLogicalPartition()) parts.push_back(&part);
  }

  return parts;
}

std::vector<const Partition_t *> Builder::getPartitions() const {
  LOGI << "Providing references of all partitions." << std::endl;
  std::vector<const Partition_t *> parts;
  for (auto &part : partitions) {
    if (!part.isLogicalPartition()) parts.push_back(&part);
  }

  return parts;
}

std::vector<Partition_t *> Builder::getLogicalPartitions() {
  LOGI << "Providing references of only logical partitions." << std::endl;
  std::vector<Partition_t *> parts;
  for (auto &part : partitions) {
    if (part.isLogicalPartition()) parts.push_back(&part);
  }

  return parts;
}

std::vector<const Partition_t *> Builder::getLogicalPartitions() const {
  LOGI << "Providing references of only logical partitions." << std::endl;
  std::vector<const Partition_t *> parts;
  for (auto &part : partitions) {
    if (part.isLogicalPartition()) parts.push_back(&part);
  }

  return parts;
}

std::vector<Partition_t *> Builder::getPartitionsByTable(const std::string &name) {
  LOGI << "Providing partitions of " << std::quoted(name) << " table." << std::endl;
  if (!tableNames.contains(name)) return {};

  std::vector<Partition_t *> parts;
  for (auto &part : partitions)
    if (!part.isLogicalPartition())
      if (part.getTableName() == name) parts.push_back(&part);

  return parts;
}

std::vector<const Partition_t *> Builder::getPartitionsByTable(const std::string &name) const {
  LOGI << "Providing partitions of " << std::quoted(name) << " table." << std::endl;
  if (!tableNames.contains(name)) return {};

  std::vector<const Partition_t *> parts;
  for (auto &part : partitions)
    if (!part.isLogicalPartition())
      if (part.getTableName() == name) parts.push_back(&part);

  return parts;
}

std::vector<std::pair<bool, std::string>> Builder::getDuplicatePartitionPositions(const std::string &name) {
  LOGI << "Building and providing (non)duplicate partition status for " << std::quoted(name) << " partition." << std::endl;
  std::vector<std::pair<bool, std::string>> parts;
  for (auto &part : partitions) {
    if (part.getName() == name) {
      if (part.isLogicalPartition()) parts.emplace_back(!part.getPathByName().empty(), "");
      else parts.emplace_back(!part.getPathByName().empty(), part.getTableName());
    }
  }

  return parts;
}

std::unordered_set<std::string> Builder::getTableNames() const {
  LOGI << "Providing all partition table list." << std::endl;
  return tableNames;
}

std::unordered_set<std::filesystem::path> Builder::getTablePaths() const {
  LOGI << "Providing all partition table path list." << std::endl;
  if (tableNames.empty()) return {};

  std::unordered_set<std::filesystem::path> paths;
  for (const auto &name : tableNames)
    paths.insert("/dev/block/" + name);

  return paths;
}

const std::map<std::filesystem::path, std::shared_ptr<GPTData>> &Builder::getAllGPTData() const {
  LOGI << "Providing GPTData structures of all partition table data." << std::endl;
  return gptDataCollection;
}

const std::shared_ptr<GPTData> &Builder::getGPTDataOf(const std::string &name) const {
  std::filesystem::path p("/dev/block");
  p /= name;
  if (!gptDataCollection.contains(p)) throw Error("Can't find GPT data of %s", name.c_str());
  LOGI << "Providing GPTData of " << std::quoted(name) << " table." << std::endl;
  return gptDataCollection.at(p);
}

std::vector<std::pair<std::string, uint64_t>> Builder::getDataOfLogicalPartitions() {
  LOGI << "Providing data of logical partitions." << std::endl;
  std::vector<std::pair<std::string, uint64_t>> parts;
  for (auto &part : getLogicalPartitions())
    parts.emplace_back(part->getName(), part->getSize());

  return parts;
}

std::vector<std::tuple<std::string, uint64_t, bool>> Builder::getDataOfPartitions() {
  LOGI << "Providing data of partitions." << std::endl;
  std::vector<std::tuple<std::string, uint64_t, bool>> parts;
  for (auto &part : getPartitions())
    parts.emplace_back(part->getName(), part->getSize(), part->isSuperPartition());

  return parts;
}

std::vector<std::tuple<std::string, uint64_t, bool>> Builder::getDataOfPartitionsByTable(const std::string &name) {
  LOGI << "Providing data of table " << std::quoted(name) << " partitions." << std::endl;
  std::vector<std::tuple<std::string, uint64_t, bool>> parts;
  for (auto &part : getPartitions())
    if (part->getTableName() == name) parts.emplace_back(part->getName(), part->getSize(), part->isSuperPartition());

  return parts;
}

Partition_t &Builder::partition(const std::string &name, const std::string &from) {
  auto it = std::ranges::find_if(partitions, [&](Partition_t &p) {
    if (p.isLogicalPartition()) return p.getName() == name;
    return from.empty() ? p.getName() == name : p.getName() == name && p.getTableName() == from;
  });
  if (it == partitions.end()) throw Error("Can't find partition with name %s", name.c_str());

  LOGI << "Providing Partition_t object of " << std::quoted(name) << " partition." << std::endl;
  return *it;
}

Partition_t &Builder::partitionWithDupCheck(const std::string &name, bool check) {
  LOGI << "Providing Partition_t object of " << std::quoted(name) << " partition with duplicate checks." << std::endl;
  auto parts = getDuplicatePartitionPositions(name);
  if (hasDuplicateNamedPartition(name) > 1 && check) {
    std::string usedName;
    std::vector<std::string> names;
    names.reserve(parts.size());

    for (const auto &[used, dname] : parts) {
      names.push_back(dname);
      if (used) usedName = dname;
    }

    while (true) {
      std::cout << std::quoted(name) << " is available on multiple tables:" << std::endl;
      for (const auto &dname : names)
        std::cout << " - " << std::quoted(dname) << std::endl;

      std::cout << "\nActively used partition " << std::quoted(name) << " is in the " << std::quoted(usedName) << " table.\n"
                << "Generally, the desired outcome is to perform operations on the currently used partition; "
                << "others are used as " << std::quoted("backup partition") << " (like xbl) or for a similar purpose.\n"
                << "Please select a table from the list above.\n>> ";

      std::string choice;
      std::cin >> choice;
      if (std::ranges::find(names, choice) != names.end()) return partition(name, choice);

      std::cout << "Invalid choice: " << std::quoted(choice) << ". Try again.\n\n";
    }
  }

  return partition(name, parts[0].second);
}

std::string Builder::getSeek() const { return seek; }

bool Builder::hasPartition(const std::string &name) {
  LOGI << "Checking " << std::quoted(name) << " named partition is exists." << std::endl;
  bool found = false;
  std::ranges::for_each(partitions, [&](auto &part) {
    if (part.getName() == name) found = true;
  });

  return found;
}

bool Builder::hasLogicalPartition(const std::string &name) {
  LOGI << "Checking " << std::quoted(name) << " logical partition is exists." << std::endl;
  bool found = false;
  std::ranges::for_each(getLogicalPartitions(), [&](auto &part) {
    if (part->getName() == name) found = true;
  });

  return found;
}

bool Builder::hasTable(const std::string &name) const {
  LOGI << "Checking " << std::quoted(name) << " partition table is exists." << std::endl;
  return tableNames.contains(name);
}

int Builder::hasDuplicateNamedPartition(const std::string &name) {
  LOGI << "Checking " << std::quoted(name) << " named partition count." << std::endl;
  int i = 0;
  std::ranges::for_each(partitions, [&](Partition_t &part) {
    if (part.getName() == name) i++;
  });

  return i;
}

bool Builder::isUsesUFS() const {
  LOGI << "Checking UFS status (used=" << std::boolalpha << isUFS << ")." << std::endl;
  return isUFS;
}

bool Builder::isHasSuperPartition() const {
  LOGI << "Checking " << std::quoted("super") << " partition is exists." << std::endl;
  for (auto part : partitions)
    if (part.getName() == "super") return true;
  return false;
}

bool Builder::isLogical(const std::string &name) { return hasLogicalPartition(name); }

bool Builder::empty() const {
  LOGI << "Providing state of this object is empty or not." << std::endl;
  return partitions.empty() && gptDataCollection.empty();
}

bool Builder::tableNamesEmpty() const { return tableNames.empty(); }

bool Builder::valid() {
  LOGI << "Validating GPTData integrity." << std::endl;
  bool hasGptProblems = false;
  Helper::Silencer silencer;
  std::ranges::for_each(gptDataCollection, [&](auto &pair) {
    if (pair.second->Verify() != 0 && pair.second->CheckHeaderValidity() != 3) hasGptProblems = true;
  });

  LOGI << "Found problem: " << std::boolalpha << hasGptProblems << std::endl;
  return !hasGptProblems;
}

bool Builder::foreach(const std::function<bool(Partition_t &)> &function) {
  LOGI << "Foreaching input function for all partitions." << std::endl;
  bool isSuccess = true;
  for (Partition_t &part : partitions)
    isSuccess &= function(part);

  return isSuccess;
}

bool Builder::foreachPartitions(const std::function<bool(Partition_t &)> &function) {
  LOGI << "Foreaching input function for normal partitions." << std::endl;
  bool isSuccess = true;
  for (auto &part : getPartitions())
    isSuccess &= function(*part);

  return isSuccess;
}

bool Builder::foreachLogicalPartitions(const std::function<bool(Partition_t &)> &function) {
  LOGI << "Foreaching input function for logical partitions." << std::endl;
  bool isSuccess = true;
  for (auto &part : getLogicalPartitions())
    isSuccess &= function(*part);

  return isSuccess;
}

bool Builder::foreachGptData(const std::function<bool(const std::filesystem::path &, std::shared_ptr<GPTData> &)> &function) {
  LOGI << "Foreaching input function for all GPTData data." << std::endl;
  bool isSuccess = true;
  for (auto &[path, gptData] : gptDataCollection)
    isSuccess &= function(path, gptData);

  return isSuccess;
}

bool Builder::foreachFor(const std::vector<std::string> &list, const std::function<bool(Partition_t &)> &function) {
  LOGI << "Foreaching input function for input list." << std::endl;
  bool isSuccess = true;
  for (auto &name : list) {
    if (hasPartition(name) || hasLogicalPartition(name)) isSuccess &= function(partition(name));
  }

  return isSuccess;
}

bool Builder::foreachForPartitions(const std::vector<std::string> &list, const std::function<bool(Partition_t &)> &function) {
  LOGI << "Foreaching input function for input list (only normal partitions)." << std::endl;
  bool isSuccess = true;
  for (auto &name : list) {
    if (hasPartition(name)) isSuccess &= function(partition(name));
  }

  return isSuccess;
}

bool Builder::foreachForLogicalPartitions(const std::vector<std::string> &list, const std::function<bool(Partition_t &)> &function) {
  LOGI << "Foreaching input function for input list (only for logical partitions)." << std::endl;
  bool isSuccess = true;
  for (auto &name : list) {
    if (hasLogicalPartition(name)) isSuccess &= function(partition(name));
  }

  return isSuccess;
}

void Builder::reScan(bool auto_toggle) {
  LOGI << "Rescanning..." << std::endl;
  if (auto_toggle) {
    if (!buildAutoOnDiskChanges) return;
  }
  scan();
  scanLogicalPartitions();
}

void Builder::addTable(const std::string &name) {
  LOGI << "Adding partition table: " << std::quoted(name) << std::endl;
  if (!tableNames.contains(name)) {
    tableNames.insert(name);
    reScan(true);
  }
}

void Builder::removeTable(const std::string &name) {
  LOGI << "Removing partition table (from list!): " << std::quoted(name) << std::endl;
  if (tableNames.contains(name)) {
    tableNames.erase(name);
    reScan(true);
  }
}

void Builder::setSeek(const std::string &name) {
  if (tableNames.contains(name)) seek = name;
}

void Builder::setTables(std::unordered_set<std::string> names) {
  LOGI << "Setting up partition table list as input list." << std::endl;
  tableNames = std::move(names);
}

void Builder::setGPTDataOf(const std::string &name, std::shared_ptr<GPTData> data) {
  LOGI << "Setting up GPTData of " << std::quoted(name) << " partition table." << std::endl;
  if (auto it = gptDataCollection.find("/dev/block/" + name); it != gptDataCollection.end()) it->second = std::move(data);
}

void Builder::setAutoScanOnTableChanges(bool state) { buildAutoOnDiskChanges = state; }

void Builder::clear() {
  LOGI << "Cleaning database." << std::endl;
  partitions.clear();
  tableNames.clear();
  gptDataCollection.clear();
}

void Builder::reset() {
  LOGI << "Trigging clear() and resetting values to defaults." << std::endl;
  clear();
  buildAutoOnDiskChanges = true;
  seek = tableNames.empty() ? "" : *tableNames.begin();
}

bool Builder::Extra::isReallyTable(const std::string &name) {
  std::filesystem::path p("/sys/class/block");
  p /= name;
  p /= "device";

  return std::filesystem::exists(p.string());
}

bool Builder::operator==(const Builder &other) {
  bool equal = true;
  std::ranges::for_each(gptDataCollection, [&](auto &pair) {
    if (pair.second->GetDiskGUID() != other.getGPTDataOf(pair.first)->GetDiskGUID()) equal = false;
  });

  return partitions == other.partitions && equal;
}

bool Builder::operator!=(const Builder &other) { return !(*this == other); }

Builder::operator bool() { return valid(); }

bool Builder::operator!() { return !valid(); }

const std::shared_ptr<GPTData> &Builder::operator[](const std::string &name) const {
  std::filesystem::path p("/dev/block/" + name);
  if (!gptDataCollection.contains(p)) throw Error("Can't find GPT data of %s", name.c_str());
  return gptDataCollection.at(p);
}

GPTPart Builder::operator[](uint32_t index) {
  if (!hasTable(seek)) return {};

  GPTPart gptPart;
  for (auto &part : partitions) {
    if (part.getTableName() == seek && part.getIndex() == index) {
      gptPart = part.getGPTPart();
      break;
    }
  }

  return gptPart;
}

} // namespace PartitionMap
