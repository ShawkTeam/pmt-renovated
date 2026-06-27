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
#include <libpartition_map/table_data_collection.hpp>
#include <libpartition_map/definations.hpp>

namespace PartitionMap {

void PartitionTableData::scan() {
  if (localTableNames.empty()) throw Error("Empty disk path.");
  Log::info("Cleaning current data and scanning partitions...");
  localPartitions.clear();
  gptDataCollection.clear();

  for (const auto &name : localTableNames) {
    std::filesystem::path p("/dev/block");
    p /= name; // Append device.

    Log::info("Silencing stdout and stderr for scanning {}...", std::quoted_string(p));
    Helper::Silencer silencer(true);
    if (auto gpt = std::make_shared<GPTData>(); gpt->LoadPartitions(p)) {
      gptDataCollection[p] = std::move(gpt); // Add to GPT data list.

      auto &storedGpt = *gptDataCollection[p];
      Log::info("Sector size of {}: {}.", std::quoted_string(p), storedGpt.GetBlockSize());
      for (uint32_t i = 0; i < storedGpt.GetNumParts(); ++i) {
        if (GPTPart part = storedGpt[i]; part.IsUsed()) {
          const std::string partPath = p.string() + (isdigit(p.string().back()) ? "p" : "") + std::to_string(i + 1);
          openpart_t *op = openpart_open(partPath.c_str(), OP_RDWR, 0);
          Partition_t _part(p, part, op, i);
          localPartitions.push_back(std::move(_part)); // Add to partition list.
          silencer.stop();
          Log::info("Registered partition: {}", part.GetDescription());
          silencer.silence();
        }
      }
    }
  }

  Log::info("Scan complete, sorting partitions by name...");
  std::ranges::sort(localPartitions, [](const Partition_t &a, const Partition_t &b) { return a.name() < b.name(); });
}

void PartitionTableData::findTablePaths() {
  Log::info("Finding partition tables in {}...", std::quoted_string("/dev/block"));
  try {
    std::vector<std::filesystem::directory_entry> entries{std::filesystem::directory_iterator("/dev/block"),
                                                          std::filesystem::directory_iterator()};
    std::ranges::sort(entries, [](const auto &a, const auto &b) { return a.path().filename() < b.path().filename(); });

    for (const auto &entry : entries) {
      if (std::filesystem::is_block_file(entry.status()) && Extra::isReallyTable(entry.path().filename())) {
        Log::info("Found partition table: {}", std::quoted_string(entry.path()));
        localTableNames.insert(entry.path().filename());
      }
    }
  } catch (const std::filesystem::filesystem_error &e) {
    throw Error("{}", e.what());
  }

  if (localTableNames.size() > 1) isUFS = true;
  if (localTableNames.empty()) throw Error("Can't find any disk or partition table in {}", std::quoted_string("/dev/block"));
  Log::info("Find complete!");
}

PartitionTableData::list_t PartitionTableData::partitions() {
  Log::info("Providing references of partitions.");
  list_t parts;
  for (auto &part : localPartitions)
    parts.push_back(std::ref(part));

  return parts;
}

PartitionTableData::const_list_t PartitionTableData::partitions() const {
  Log::info("Providing references of partitions.");
  const_list_t parts;
  for (auto &part : localPartitions)
    parts.push_back(std::cref(part));

  return parts;
}

PartitionTableData::list_t PartitionTableData::partitionsByTable(const std::string &name) {
  Log::info("Providing partitions of {} table.", std::quoted_string(name));
  if (!localTableNames.contains(name)) return {};

  list_t parts;
  for (auto &part : localPartitions)
    if (part.tableName() == name) parts.push_back(std::ref(part));

  return parts;
}

PartitionTableData::const_list_t PartitionTableData::partitionsByTable(const std::string &name) const {
  Log::info("Providing partitions of {} table.", std::quoted_string(name));
  if (!localTableNames.contains(name)) return {};

  const_list_t parts;
  for (auto &part : localPartitions)
    if (part.tableName() == name) parts.push_back(std::cref(part));

  return parts;
}

std::vector<std::pair<bool, std::string>> PartitionTableData::duplicatePartitionPositions(const std::string &name) const {
  Log::info("Building and providing (non)duplicate partition status for {} partition.", std::quoted_string(name));
  std::vector<std::pair<bool, std::string>> parts;
  for (auto &part : localPartitions) {
    if (part.name() == name) parts.emplace_back(!part.pathByName().empty(), part.tableName());
  }

  return parts;
}

const std::unordered_set<std::string> &PartitionTableData::tableNames() const {
  Log::info("Providing all partition table list.");
  return localTableNames;
}

std::unordered_set<std::string> &PartitionTableData::tableNames() {
  Log::info("Providing all partition table list.");
  return localTableNames;
}

std::unordered_set<std::filesystem::path> PartitionTableData::tablePaths() const {
  Log::info("Providing all partition table path list.");
  if (localTableNames.empty()) return {};

  std::unordered_set<std::filesystem::path> paths;
  for (const auto &name : localTableNames)
    paths.insert("/dev/block/" + name);

  return paths;
}

const std::map<std::filesystem::path, std::shared_ptr<GPTData>> &PartitionTableData::allGPTData() const {
  Log::info("Providing all GPTData.");
  return gptDataCollection;
}

const std::shared_ptr<GPTData> &PartitionTableData::GPTDataOf(const std::string &name) const {
  std::filesystem::path p("/dev/block");
  p /= name;
  if (!gptDataCollection.contains(p)) throw Error("Can't find GPT data of {}", name);
  Log::info("Providing GPTData of {} table.", std::quoted_string(name));
  return gptDataCollection.at(p);
}

std::shared_ptr<GPTData> &PartitionTableData::GPTDataOf(const std::string &name) {
  std::filesystem::path p("/dev/block");
  p /= name;
  if (!gptDataCollection.contains(p)) throw Error("Can't find GPT data of {}", name);
  Log::info("Providing GPTData of {} table.", std::quoted_string(name));
  return gptDataCollection.at(p);
}

std::vector<BasicInfo> PartitionTableData::aboutPartitions() const {
  Log::info("Providing data of partitions.");
  std::vector<BasicInfo> parts;
  for (const auto &p : partitions()) {
    const Partition_t &part = p;
    parts.emplace_back(part.name(), part.size(), false);
  }

  return parts;
}

std::vector<BasicInfo> PartitionTableData::aboutPartitionsByTable(const std::string &name) const {
  Log::info("Providing data of partitions of {} table.", std::quoted_string(name));
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

  Log::info("Providing Partition_t object of {} partition.", std::quoted_string(name));
  return std::ref(*it);
}

std::optional<std::reference_wrapper<const Partition_t>> PartitionTableData::partition(const std::string &name,
                                                                                       const std::string &from) const {
  auto it = std::ranges::find_if(localPartitions, [&](const Partition_t &p) {
    return from.empty() ? p.name() == name : p.name() == name && p.tableName() == from;
  });
  if (it == localPartitions.end()) return std::nullopt;

  Log::info("Providing Partition_t object of {} partition.", std::quoted_string(name));
  return std::cref(*it);
}

std::optional<std::reference_wrapper<Partition_t>> PartitionTableData::partitionWithDupCheck(const std::string &name, bool check) {
  Log::info("Providing Partition_t object of {} partition with duplicate checks.", std::quoted_string(name));
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
  Log::info("Providing Partition_t object of {} partition with duplicate checks.", std::quoted_string(name));
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
  Log::info("Checking {} named partition is exists.", std::quoted_string(name));
  bool found = false;
  std::ranges::for_each(localPartitions, [&](auto &part) {
    if (part.name() == name) found = true;
  });

  return found;
}

bool PartitionTableData::hasTable(const std::string &name) const {
  Log::info("Checking {} partition table is exists.", std::quoted_string(name));
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
  Log::info("Checking {} named partition count.", std::quoted_string(name));
  int i = 0;
  std::ranges::for_each(localPartitions, [&](const Partition_t &part) {
    if (part.name() == name) i++;
  });

  return i;
}

bool PartitionTableData::isUsesUFS() const {
  Log::info("Checking UFS status (used={}).", isUFS ? "true" : "false");
  return isUFS;
}

bool PartitionTableData::isHasSuperPartition() const {
  Log::info("Checking \"super\" partition is exists.");
  return std::ranges::any_of(localPartitions, [](const auto &part) { return part.name() == "super"; });
}

bool PartitionTableData::empty() const {
  Log::info("Checking state of this object is empty or not.");
  return localPartitions.empty() && gptDataCollection.empty();
}

bool PartitionTableData::tableNamesEmpty() const { return localTableNames.empty(); }

bool PartitionTableData::valid() const {
  Log::info("Checking GPTData integrity.");
  bool hasGptProblems = false;
  Helper::Silencer silencer;
  std::ranges::for_each(gptDataCollection, [&](auto &pair) {
    if (pair.second->Verify() != 0 && pair.second->CheckHeaderValidity() != 3) {
      Log::error("FOUND PROBLEMS ON {}", pair.first.string());
      hasGptProblems = true;
    }
  });

  Log::info("Found problem: {}", hasGptProblems ? "true" : "false");
  return !hasGptProblems;
}

void PartitionTableData::reScan(bool auto_toggled) {
  Log::info("[PRIVATE] Rescanning...");
  if (auto_toggled) {
    if (!buildAutoOnDiskChanges) return;
  }
  scan();
}

void PartitionTableData::reScan() {
  Log::info("Rescanning...");
  scan();
}

void PartitionTableData::removeTable(const std::string &name) {
  Log::info("Removing partition table (from list!): {}", std::quoted_string(name));
  if (localTableNames.contains(name)) {
    localTableNames.erase(name);
    reScan(true);
  }
}

void PartitionTableData::setTables(const std::unordered_set<std::string> &tables) {
  Log::info("Setting up partition table list as input list.");
  localTableNames = tables;
}

PartitionTableData &PartitionTableData::withTables(const std::unordered_set<std::string> &tables) {
  Log::info("Setting up partition table list as input list.");
  localTableNames = tables;
  return *this;
}

void PartitionTableData::setGPTDataOf(const std::string &name, std::shared_ptr<GPTData> data) {
  Log::info("Setting up GPTData of {} partition table.", std::quoted_string(name));
  if (auto it = gptDataCollection.find("/dev/block/" + name); it != gptDataCollection.end()) it->second = std::move(data);
}

void PartitionTableData::setAutoScan(bool state) { buildAutoOnDiskChanges = state; }

PartitionTableData &PartitionTableData::withAutoScan(bool state) {
  buildAutoOnDiskChanges = state;
  return *this;
}

void PartitionTableData::clear() {
  Log::info("Cleaning.");
  localPartitions.clear();
  localTableNames.clear();
  gptDataCollection.clear();
}

void PartitionTableData::reset() {
  Log::info("Calling clear() and resetting values to defaults.");
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
