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
#include <iostream>
#include <ranges>
#include <utility>
#include <libhelper/management.hpp>
#include <libhelper/functions.hpp>
#include <libpartition_map/table_data_collection.hpp>
#include <libpartition_map/definations.hpp>
#include <libpartition_map/redefine_logging_macros.hpp>

namespace PartitionMap {

void PartitionTableData::scan() {
  if (localTableNames.empty()) throw Error("Empty disk path.");
  LOGI << "Cleaning current data and scanning partitions..." << std::endl;
  localPartitions.clear();
  gptDataCollection.clear();

  for (const auto &name : localTableNames) {
    std::filesystem::path p("/dev/block");
    p /= name; // Append device.

    LOGI << "Silencing stdout and stderr for scanning " << std::quoted(p.string()) << "..." << std::endl << std::flush;
    Helper::Silencer silencer(true);
    if (auto gpt = std::make_shared<GPTData>(); gpt->LoadPartitions(p)) {
      auto sectorSize = gpt->GetBlockSize();
      gptDataCollection[p] = std::move(gpt); // Add to GPT data list.

      auto &storedGpt = *gptDataCollection[p];
      LOGI << "Sector size of " << std::quoted(p.string()) << ": " << sectorSize << std::endl << std::flush;
      for (uint32_t i = 0; i < storedGpt.GetNumParts(); ++i) {
        if (GPTPart part = storedGpt[i]; part.IsUsed()) {
          Partition_t _part(p, part, i);
          _part.setDefaultSectorSize(sectorSize);
          localPartitions.push_back(std::move(_part)); // Add to partition list.
          silencer.stop();
          LOGI << "Registered partition: " << part.GetDescription() << std::endl << std::flush;
          silencer.silence();
        }
      }
    }
  }

  LOGI << "Scan complete!" << std::endl;
  LOGI << "Sorting partitions by name." << std::endl;
  std::ranges::sort(localPartitions, [](const Partition_t &a, const Partition_t &b) { return a.name() < b.name(); });
}

void PartitionTableData::findTablePaths() {
  LOGI << "Finding partition tables in " << std::quoted("/dev/block") << std::endl;
  try {
    std::vector<std::filesystem::directory_entry> entries{std::filesystem::directory_iterator("/dev/block"),
                                                          std::filesystem::directory_iterator()};
    std::ranges::sort(entries, [](const auto &a, const auto &b) { return a.path().filename() < b.path().filename(); });

    for (const auto &entry : entries) {
      if (std::filesystem::is_block_file(entry.status()) && Extra::isReallyTable(entry.path().filename())) {
        LOGI << "Found partition table: " << entry.path() << std::endl;
        localTableNames.insert(entry.path().filename());
      }
    }
  } catch (const std::filesystem::filesystem_error &e) {
    throw Error("{}", e.what());
  }

  LOGI << "Find complete!" << std::endl;
  if (localTableNames.size() > 1) isUFS = true;
  if (localTableNames.empty()) throw Error("Can't find any disk or partition table in {}", std::quoted_string("/dev/block"));
}

PartitionTableData::list_t PartitionTableData::partitions() {
  LOGI << "Providing references of partitions." << std::endl;
  list_t parts;
  for (auto &part : localPartitions)
    parts.push_back(std::ref(part));

  return parts;
}

PartitionTableData::const_list_t PartitionTableData::partitions() const {
  LOGI << "Providing references of partitions." << std::endl;
  const_list_t parts;
  for (auto &part : localPartitions)
    parts.push_back(std::cref(part));

  return parts;
}

PartitionTableData::list_t PartitionTableData::partitionsByTable(const std::string &name) {
  LOGI << "Providing partitions of " << std::quoted(name) << " table." << std::endl;
  if (!localTableNames.contains(name)) return {};

  list_t parts;
  for (auto &part : localPartitions)
    if (part.tableName() == name) parts.push_back(std::ref(part));

  return parts;
}

PartitionTableData::const_list_t PartitionTableData::partitionsByTable(const std::string &name) const {
  LOGI << "Providing partitions of " << std::quoted(name) << " table." << std::endl;
  if (!localTableNames.contains(name)) return {};

  const_list_t parts;
  for (auto &part : localPartitions)
    if (part.tableName() == name) parts.push_back(std::cref(part));

  return parts;
}

std::vector<std::pair<bool, std::string>> PartitionTableData::duplicatePartitionPositions(const std::string &name) const {
  LOGI << "Building and providing (non)duplicate partition status for " << std::quoted(name) << " partition." << std::endl;
  std::vector<std::pair<bool, std::string>> parts;
  for (auto &part : localPartitions) {
    if (part.name() == name) parts.emplace_back(!part.pathByName().empty(), part.tableName());
  }

  return parts;
}

const std::unordered_set<std::string> &PartitionTableData::tableNames() const {
  LOGI << "Providing all partition table list." << std::endl;
  return localTableNames;
}

std::unordered_set<std::string> &PartitionTableData::tableNames() {
  LOGI << "Providing all partition table list." << std::endl;
  return localTableNames;
}

std::unordered_set<std::filesystem::path> PartitionTableData::tablePaths() const {
  LOGI << "Providing all partition table path list." << std::endl;
  if (localTableNames.empty()) return {};

  std::unordered_set<std::filesystem::path> paths;
  for (const auto &name : localTableNames)
    paths.insert("/dev/block/" + name);

  return paths;
}

const std::map<std::filesystem::path, std::shared_ptr<GPTData>> &PartitionTableData::allGPTData() const {
  LOGI << "Providing GPTData structures of all partition table data." << std::endl;
  return gptDataCollection;
}

const std::shared_ptr<GPTData> &PartitionTableData::GPTDataOf(const std::string &name) const {
  std::filesystem::path p("/dev/block");
  p /= name;
  if (!gptDataCollection.contains(p)) throw Error("Can't find GPT data of {}", name);
  LOGI << "Providing GPTData of " << std::quoted(name) << " table." << std::endl;
  return gptDataCollection.at(p);
}

std::shared_ptr<GPTData> &PartitionTableData::GPTDataOf(const std::string &name) {
  std::filesystem::path p("/dev/block");
  p /= name;
  if (!gptDataCollection.contains(p)) throw Error("Can't find GPT data of {}", name);
  LOGI << "Providing GPTData of " << std::quoted(name) << " table." << std::endl;
  return gptDataCollection.at(p);
}

std::vector<BasicInfo> PartitionTableData::aboutPartitions() const {
  LOGI << "Providing data of partitions." << std::endl;
  std::vector<BasicInfo> parts;
  for (const auto &p : partitions()) {
    const Partition_t &part = p;
    parts.emplace_back(part.name(), part.size(), false);
  }

  return parts;
}

std::vector<BasicInfo> PartitionTableData::aboutPartitionsByTable(const std::string &name) const {
  LOGI << "Providing data of table " << std::quoted(name) << " partitions." << std::endl;
  std::vector<BasicInfo> parts;
  for (const auto &p : partitions()) {
    const Partition_t &part = p;
    if (part.tableName() == name) parts.emplace_back(part.name(), part.size(), false);
  }

  return parts;
}

std::optional<std::reference_wrapper<Partition_t>> PartitionTableData::partition(const std::string &name, const std::string &from) {
  auto it = std::ranges::find_if(localPartitions, [&](const Partition_t &p) {
    return from.empty() ? p.name() == name : p.name() == name && p.tableName() == from;
  });
  if (it == localPartitions.end()) return std::nullopt;

  LOGI << "Providing Partition_t object of " << std::quoted(name) << " partition." << std::endl;
  return std::ref(*it);
}

std::optional<std::reference_wrapper<const Partition_t>> PartitionTableData::partition(const std::string &name,
                                                                                       const std::string &from) const {
  auto it = std::ranges::find_if(localPartitions, [&](const Partition_t &p) {
    return from.empty() ? p.name() == name : p.name() == name && p.tableName() == from;
  });
  if (it == localPartitions.end()) return std::nullopt;

  LOGI << "Providing Partition_t object of " << std::quoted(name) << " partition." << std::endl;
  return std::cref(*it);
}

std::optional<std::reference_wrapper<Partition_t>> PartitionTableData::partitionWithDupCheck(const std::string &name, bool check) {
  LOGI << "Providing Partition_t object of " << std::quoted(name) << " partition with duplicate checks." << std::endl;
  auto parts = duplicatePartitionPositions(name);
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

std::optional<std::reference_wrapper<const Partition_t>> PartitionTableData::partitionWithDupCheck(const std::string &name,
                                                                                                   bool check) const {
  LOGI << "Providing Partition_t object of " << std::quoted(name) << " partition with duplicate checks." << std::endl;
  auto parts = duplicatePartitionPositions(name);
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

bool PartitionTableData::hasPartition(const std::string &name) const {
  LOGI << "Checking " << std::quoted(name) << " named partition is exists." << std::endl;
  bool found = false;
  std::ranges::for_each(localPartitions, [&](auto &part) {
    if (part.name() == name) found = true;
  });

  return found;
}

bool PartitionTableData::hasTable(const std::string &name) const {
  LOGI << "Checking " << std::quoted(name) << " partition table is exists." << std::endl;
  return localTableNames.contains(name);
}

uint64_t PartitionTableData::freeSpaceOf(const std::string &name) const {
  if (!localTableNames.contains(name)) return std::numeric_limits<uint64_t>::max();

  const auto &data = GPTDataOf(name);
  uint32_t numSegments = 0;
  uint64_t largestSegment = 0;
  uint64_t totalFreeSectors = data->FindFreeBlocks(&numSegments, &largestSegment);

  return totalFreeSectors * data->GetBlockSize();
}

int PartitionTableData::hasDuplicateNamedPartition(const std::string &name) const {
  LOGI << "Checking " << std::quoted(name) << " named partition count." << std::endl;
  int i = 0;
  std::ranges::for_each(localPartitions, [&](const Partition_t &part) {
    if (part.name() == name) i++;
  });

  return i;
}

bool PartitionTableData::isUsesUFS() const {
  LOGI << "Checking UFS status (used=" << std::boolalpha << isUFS << ")." << std::endl;
  return isUFS;
}

bool PartitionTableData::isHasSuperPartition() const {
  LOGI << "Checking " << std::quoted("super") << " partition is exists." << std::endl;

  return std::ranges::any_of(localPartitions, [](const auto &part) { return part.name() == "super"; });
}

bool PartitionTableData::empty() const {
  LOGI << "Providing state of this object is empty or not." << std::endl;
  return localPartitions.empty() && gptDataCollection.empty();
}

bool PartitionTableData::tableNamesEmpty() const { return localTableNames.empty(); }

bool PartitionTableData::valid() const {
  LOGI << "Validating GPTData integrity." << std::endl;
  bool hasGptProblems = false;
  Helper::Silencer silencer;
  std::ranges::for_each(gptDataCollection, [&](auto &pair) {
    if (pair.second->Verify() != 0 && pair.second->CheckHeaderValidity() != 3) {
      LOGE << "FOUND PROBLEMS ON " << pair.first << std::endl;
      hasGptProblems = true;
    }
  });

  LOGI << "Found problem: " << std::boolalpha << hasGptProblems << std::endl;
  return !hasGptProblems;
}

void PartitionTableData::reScan(bool auto_toggled) {
  LOGI << "[PRIVATE FUNCTION] Rescanning..." << std::endl;
  if (auto_toggled) {
    if (!buildAutoOnDiskChanges) return;
  }
  scan();
}

void PartitionTableData::reScan() {
  LOGI << "Rescanning..." << std::endl;
  scan();
}

void PartitionTableData::removeTable(const std::string &name) {
  LOGI << "Removing partition table (from list!): " << std::quoted(name) << std::endl;
  if (localTableNames.contains(name)) {
    localTableNames.erase(name);
    reScan(true);
  }
}

void PartitionTableData::setTables(const std::unordered_set<std::string> &tables) {
  LOGI << "Setting up partition table list as input list." << std::endl;
  localTableNames = tables;
}

PartitionTableData &PartitionTableData::withTables(const std::unordered_set<std::string> &tables) {
  LOGI << "Setting up partition table list as input list." << std::endl;
  localTableNames = tables;
  return *this;
}

void PartitionTableData::setGPTDataOf(const std::string &name, std::shared_ptr<GPTData> data) {
  LOGI << "Setting up GPTData of " << std::quoted(name) << " partition table." << std::endl;
  if (auto it = gptDataCollection.find("/dev/block/" + name); it != gptDataCollection.end()) it->second = std::move(data);
}

void PartitionTableData::setAutoScan(bool state) { buildAutoOnDiskChanges = state; }

PartitionTableData &PartitionTableData::withAutoScan(bool state) {
  buildAutoOnDiskChanges = state;
  return *this;
}

void PartitionTableData::clear() {
  LOGI << "Cleaning database." << std::endl;
  localPartitions.clear();
  localTableNames.clear();
  gptDataCollection.clear();
}

void PartitionTableData::reset() {
  LOGI << "Trigging clear() and resetting values to defaults." << std::endl;
  clear();
  buildAutoOnDiskChanges = true;
}

bool PartitionTableData::Extra::isReallyTable(const std::string &name) {
  std::filesystem::path p("/sys/class/block");
  p /= name;
  p /= "device";

  return std::filesystem::exists(p.string());
}

PartitionTableData::iterator PartitionTableData::begin() { return localPartitions.begin(); }
PartitionTableData::iterator PartitionTableData::end() { return localPartitions.end(); }

PartitionTableData::const_iterator PartitionTableData::begin() const { return localPartitions.begin(); }
PartitionTableData::const_iterator PartitionTableData::end() const { return localPartitions.end(); }
PartitionTableData::const_iterator PartitionTableData::cbegin() const { return localPartitions.cbegin(); }
PartitionTableData::const_iterator PartitionTableData::cend() const { return localPartitions.cend(); }

bool PartitionTableData::operator==(const PartitionTableData &other) const {
  bool equal = true;
  std::ranges::for_each(gptDataCollection, [&](auto &pair) {
    if (pair.second->GetDiskGUID() != other.GPTDataOf(pair.first)->GetDiskGUID()) equal = false;
  });

  return localPartitions == other.localPartitions && equal;
}

bool PartitionTableData::operator!=(const PartitionTableData &other) const { return !(*this == other); }

PartitionTableData::operator bool() const { return valid(); }

bool PartitionTableData::operator!() const { return !valid(); }

PartitionTableData::const_list_t PartitionTableData::operator*() const {
  const_list_t parts;
  parts.reserve(localPartitions.size());
  for (const auto &part : localPartitions)
    parts.push_back(std::cref(part));

  return parts;
}

PartitionTableData::list_t PartitionTableData::operator*() {
  list_t parts;
  parts.reserve(localPartitions.size());
  for (auto &part : localPartitions)
    parts.push_back(std::ref(part));

  return parts;
}

const std::shared_ptr<GPTData> &PartitionTableData::operator[](const std::string &name) const {
  const std::filesystem::path p("/dev/block/" + name);
  if (!gptDataCollection.contains(p)) throw Error("Can't find GPT data of {}", name);
  return gptDataCollection.at(p);
}

std::shared_ptr<GPTData> &PartitionTableData::operator[](const std::string &name) {
  const std::filesystem::path p("/dev/block/" + name);
  if (!gptDataCollection.contains(p)) throw Error("Can't find GPT data of {}", name);
  return gptDataCollection.at(p);
}

GPTPart *PartitionTableData::operator()(const std::string &name, uint32_t index) {
  if (!hasTable(name)) return nullptr;

  std::string name_;
  for (auto &part : localPartitions) {
    if (part.tableName() == name && part.index() == index) {
      name_ = part.name();
      break;
    }
  }

  return partition(name_)->get().getGPTPartRef();
}

const GPTPart *PartitionTableData::operator()(const std::string &name, uint32_t index) const {
  if (!hasTable(name)) return nullptr;

  std::string name_;
  for (auto &part : localPartitions) {
    if (part.tableName() == name && part.index() == index) {
      name_ = part.name();
      break;
    }
  }

  return partition(name_)->get().getGPTPartRef();
}

PartitionTableData &PartitionTableData::operator=(PartitionTableData &&other) noexcept {
  if (this != &other) {
    localPartitions = std::move(other.localPartitions);
    gptDataCollection = std::move(other.gptDataCollection);
    localTableNames = std::move(other.localTableNames);
    buildAutoOnDiskChanges = other.buildAutoOnDiskChanges;
    isUFS = other.isUFS;

    other.buildAutoOnDiskChanges = true;
    other.isUFS = false;
  }

  return *this;
}

} // namespace PartitionMap
