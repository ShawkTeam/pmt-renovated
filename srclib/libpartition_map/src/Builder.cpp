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
#include <libpartition_map/lib.hpp>
#include <libpartition_map/redefine_logging_macros.hpp>

namespace PartitionMap {

void Builder::scan() {
  if (localTableNames.empty()) throw ERR << "Empty disk path.";
  LOGI << "Cleaning current data and scanning partitions..." << std::endl;
  localPartitions.clear();
  gptDataCollection.clear();

  for (const auto &name : localTableNames) {
    std::filesystem::path p("/dev/block");
    p /= name; // Append device.

    LOGI << "Silencing stdout..." << std::endl << std::flush;
    Helper::Silencer silencer;
    if (auto gpt = std::make_shared<GPTData>(); gpt->LoadPartitions(p)) {
      gptDataCollection[p] = std::move(gpt); // Add to GPT data list.

      auto &storedGpt = *gptDataCollection[p];
      for (uint32_t i = 0; i < storedGpt.GetNumParts(); ++i) {
        if (GPTPart part = storedGpt[i]; part.IsUsed()) {
          localPartitions.push_back(Partition_t({part, i, p})); // Add to partition list.
          silencer.stop();
          LOGI << "Registered partition: " << part.GetDescription() << std::endl << std::flush;
          silencer.silenceAgain();
        }
      }
    }
  }

  LOGI << "Scan complete!" << std::endl;
  LOGI << "Sorting partitions by name." << std::endl;
  std::ranges::sort(localPartitions, [](const Partition_t &a, const Partition_t &b) { return a.name() < b.name(); });
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
      localPartitions.emplace_back(entry.path());
      LOGI << "Registered logical partition: " << entry.path().filename() << std::endl;
    }
  }

  LOGI << "Scan complete!" << std::endl;
  LOGI << "Removing non-partition contents from data." << std::endl;
  std::erase_if(localPartitions, [](const Partition_t &part) -> bool {
    if (part.name().find("com.") != std::string::npos ||
        part.name() == "userdata") { // Erase non-partition contents (like com.android.adbd) and userdata.
      LOGI << "Removed: " << part.name() << std::endl;
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
        localTableNames.insert(entry.path().filename());
      }
    }
  } catch (const std::filesystem::filesystem_error &e) {
    throw ERR << e.what();
  }

  LOGI << "Find complete!" << std::endl;
  if (localTableNames.size() > 1) isUFS = true;
  if (localTableNames.empty()) throw ERR << "Can't find any disk or partition table in " << std::quoted("/dev/block");
}

std::vector<Partition_t *> Builder::allPartitions() {
  LOGI << "Providing references of all partitions." << std::endl;
  std::vector<Partition_t *> parts;
  parts.reserve(logicalPartitions().size());
  for (auto &part : localPartitions)
    parts.push_back(&part);

  return parts;
}

std::vector<const Partition_t *> Builder::allPartitions() const {
  LOGI << "Providing references of all partitions." << std::endl;
  std::vector<const Partition_t *> parts;
  parts.reserve(localPartitions.size());
  for (auto &part : localPartitions)
    parts.push_back(&part);

  return parts;
}

std::vector<Partition_t *> Builder::partitions() {
  LOGI << "Providing references of all normal partitions." << std::endl;
  std::vector<Partition_t *> parts;
  for (auto &part : localPartitions) {
    if (!part.isLogicalPartition()) parts.push_back(&part);
  }

  return parts;
}

std::vector<const Partition_t *> Builder::partitions() const {
  LOGI << "Providing references of all normal partitions." << std::endl;
  std::vector<const Partition_t *> parts;
  for (auto &part : localPartitions) {
    if (!part.isLogicalPartition()) parts.push_back(&part);
  }

  return parts;
}

std::vector<Partition_t *> Builder::logicalPartitions() {
  LOGI << "Providing references of only logical partitions." << std::endl;
  std::vector<Partition_t *> parts;
  for (auto &part : localPartitions) {
    if (part.isLogicalPartition()) parts.push_back(&part);
  }

  return parts;
}

std::vector<const Partition_t *> Builder::logicalPartitions() const {
  LOGI << "Providing references of only logical partitions." << std::endl;
  std::vector<const Partition_t *> parts;
  for (auto &part : localPartitions) {
    if (part.isLogicalPartition()) parts.push_back(&part);
  }

  return parts;
}

std::vector<Partition_t *> Builder::partitionsByTable(const std::string &name) {
  LOGI << "Providing partitions of " << std::quoted(name) << " table." << std::endl;
  if (!localTableNames.contains(name)) return {};

  std::vector<Partition_t *> parts;
  for (auto &part : localPartitions)
    if (!part.isLogicalPartition())
      if (part.tableName() == name) parts.push_back(&part);

  return parts;
}

std::vector<const Partition_t *> Builder::partitionsByTable(const std::string &name) const {
  LOGI << "Providing partitions of " << std::quoted(name) << " table." << std::endl;
  if (!localTableNames.contains(name)) return {};

  std::vector<const Partition_t *> parts;
  for (auto &part : localPartitions)
    if (!part.isLogicalPartition())
      if (part.tableName() == name) parts.push_back(&part);

  return parts;
}

std::vector<std::pair<bool, std::string>> Builder::duplicatePartitionPositions(const std::string &name) const {
  LOGI << "Building and providing (non)duplicate partition status for " << std::quoted(name) << " partition." << std::endl;
  std::vector<std::pair<bool, std::string>> parts;
  for (auto &part : localPartitions) {
    if (part.name() == name) {
      if (part.isLogicalPartition())
        parts.emplace_back(!part.pathByName().empty(), "");
      else
        parts.emplace_back(!part.pathByName().empty(), part.tableName());
    }
  }

  return parts;
}

const std::unordered_set<std::string> &Builder::tableNames() const {
  LOGI << "Providing all partition table list." << std::endl;
  return localTableNames;
}

std::unordered_set<std::string> &Builder::tableNames() {
  LOGI << "Providing all partition table list." << std::endl;
  return localTableNames;
}

std::unordered_set<std::filesystem::path> Builder::tablePaths() const {
  LOGI << "Providing all partition table path list." << std::endl;
  if (localTableNames.empty()) return {};

  std::unordered_set<std::filesystem::path> paths;
  for (const auto &name : localTableNames)
    paths.insert("/dev/block/" + name);

  return paths;
}

const std::map<std::filesystem::path, std::shared_ptr<GPTData>> &Builder::allGPTData() const {
  LOGI << "Providing GPTData structures of all partition table data." << std::endl;
  return gptDataCollection;
}

const std::shared_ptr<GPTData> &Builder::GPTDataOf(const std::string &name) const {
  std::filesystem::path p("/dev/block");
  p /= name;
  if (!gptDataCollection.contains(p)) throw ERR << "Can't find GPT data of " << name;
  LOGI << "Providing GPTData of " << std::quoted(name) << " table." << std::endl;
  return gptDataCollection.at(p);
}

std::shared_ptr<GPTData> &Builder::GPTDataOf(const std::string &name) {
  std::filesystem::path p("/dev/block");
  p /= name;
  if (!gptDataCollection.contains(p)) throw ERR << "Can't find GPT data of " << name;
  LOGI << "Providing GPTData of " << std::quoted(name) << " table." << std::endl;
  return gptDataCollection.at(p);
}

std::vector<std::pair<std::string, uint64_t>> Builder::dataOfLogicalPartitions() {
  LOGI << "Providing data of logical partitions." << std::endl;
  std::vector<std::pair<std::string, uint64_t>> parts;
  for (const auto &part : logicalPartitions())
    parts.emplace_back(part->name(), part->size());

  return parts;
}

std::vector<BasicInfo> Builder::dataOfPartitions() {
  LOGI << "Providing data of partitions." << std::endl;
  std::vector<BasicInfo> parts;
  for (const auto &part : partitions())
    parts.emplace_back(part->name(), part->size(), part->isSuperPartition());

  return parts;
}

std::vector<BasicInfo> Builder::dataOfPartitionsByTable(const std::string &name) {
  LOGI << "Providing data of table " << std::quoted(name) << " partitions." << std::endl;
  std::vector<BasicInfo> parts;
  for (const auto &part : partitions())
    if (part->tableName() == name) parts.emplace_back(part->name(), part->size(), part->isSuperPartition());

  return parts;
}

Partition_t &Builder::partition(const std::string &name, const std::string &from) {
  auto it = std::ranges::find_if(localPartitions, [&](const Partition_t &p) {
    if (p.isLogicalPartition()) return p.name() == name;
    return from.empty() ? p.name() == name : p.name() == name && p.tableName() == from;
  });
  if (it == localPartitions.end()) throw ERR << "Can't find partition with name " << name;

  LOGI << "Providing Partition_t object of " << std::quoted(name) << " partition." << std::endl;
  return *it;
}

const Partition_t &Builder::partition(const std::string &name, const std::string &from) const {
  auto it = std::ranges::find_if(localPartitions, [&](const Partition_t &p) {
    if (p.isLogicalPartition()) return p.name() == name;
    return from.empty() ? p.name() == name : p.name() == name && p.tableName() == from;
  });
  if (it == localPartitions.end()) throw ERR << "Can't find partition with name " << name;

  LOGI << "Providing Partition_t object of " << std::quoted(name) << " partition." << std::endl;
  return *it;
}

Partition_t &Builder::partitionWithDupCheck(const std::string &name, bool check) {
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

const Partition_t &Builder::partitionWithDupCheck(const std::string &name, bool check) const {
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

bool Builder::hasPartition(const std::string &name) const {
  LOGI << "Checking " << std::quoted(name) << " named partition is exists." << std::endl;
  bool found = false;
  std::ranges::for_each(localPartitions, [&](auto &part) {
    if (part.name() == name) found = true;
  });

  return found;
}

bool Builder::hasLogicalPartition(const std::string &name) const {
  LOGI << "Checking " << std::quoted(name) << " logical partition is exists." << std::endl;
  bool found = false;
  std::ranges::for_each(logicalPartitions(), [&](auto &part) {
    if (part->name() == name) found = true;
  });

  return found;
}

bool Builder::hasTable(const std::string &name) const {
  LOGI << "Checking " << std::quoted(name) << " partition table is exists." << std::endl;
  return localTableNames.contains(name);
}

int Builder::hasDuplicateNamedPartition(const std::string &name) const {
  LOGI << "Checking " << std::quoted(name) << " named partition count." << std::endl;
  int i = 0;
  std::ranges::for_each(localPartitions, [&](const Partition_t &part) {
    if (part.name() == name) i++;
  });

  return i;
}

bool Builder::isUsesUFS() const {
  LOGI << "Checking UFS status (used=" << std::boolalpha << isUFS << ")." << std::endl;
  return isUFS;
}

bool Builder::isHasSuperPartition() const {
  LOGI << "Checking " << std::quoted("super") << " partition is exists." << std::endl;

  return std::ranges::any_of(localPartitions, [](const auto &part) { return part.name() == "super"; });
}

bool Builder::isLogical(const std::string &name) const { return hasLogicalPartition(name); }

bool Builder::empty() const {
  LOGI << "Providing state of this object is empty or not." << std::endl;
  return localPartitions.empty() && gptDataCollection.empty();
}

bool Builder::tableNamesEmpty() const { return localTableNames.empty(); }

bool Builder::valid() const {
  LOGI << "Validating GPTData integrity." << std::endl;
  bool hasGptProblems = false;
  Helper::Silencer silencer;
  std::ranges::for_each(gptDataCollection, [&](auto &pair) {
    if (pair.second->Verify() != 0 && pair.second->CheckHeaderValidity() != 3) hasGptProblems = true;
  });

  LOGI << "Found problem: " << std::boolalpha << hasGptProblems << std::endl;
  return !hasGptProblems;
}

bool Builder::foreach (const std::function<bool(Partition_t &)> &function) {
  LOGI << "Foreaching input function for all partitions." << std::endl;
  bool isSuccess = true;
  for (auto &part : localPartitions)
    isSuccess &= function(part);

  return isSuccess;
}

bool Builder::foreach (const std::function<bool(const Partition_t &)> &function) const {
  LOGI << "Foreaching input function for all partitions." << std::endl;
  bool isSuccess = true;
  for (const auto &part : localPartitions)
    isSuccess &= function(part);

  return isSuccess;
}

bool Builder::foreachPartitions(const std::function<bool(Partition_t &)> &function) {
  LOGI << "Foreaching input function for normal partitions." << std::endl;
  bool isSuccess = true;
  for (auto &part : partitions())
    isSuccess &= function(*part);

  return isSuccess;
}

bool Builder::foreachPartitions(const std::function<bool(const Partition_t &)> &function) const {
  LOGI << "Foreaching input function for normal partitions." << std::endl;
  bool isSuccess = true;
  for (const auto &part : partitions())
    isSuccess &= function(*part);

  return isSuccess;
}

bool Builder::foreachLogicalPartitions(const std::function<bool(Partition_t &)> &function) {
  LOGI << "Foreaching input function for logical partitions." << std::endl;
  bool isSuccess = true;
  for (auto &part : logicalPartitions())
    isSuccess &= function(*part);

  return isSuccess;
}

bool Builder::foreachLogicalPartitions(const std::function<bool(const Partition_t &)> &function) const {
  LOGI << "Foreaching input function for logical partitions." << std::endl;
  bool isSuccess = true;
  for (const auto &part : logicalPartitions())
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

bool Builder::foreachGptData(
    const std::function<bool(const std::filesystem::path &, const std::shared_ptr<GPTData> &)> &function) const {
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

bool Builder::foreachFor(const std::vector<std::string> &list, const std::function<bool(const Partition_t &)> &function) const {
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

bool Builder::foreachForPartitions(const std::vector<std::string> &list,
                                   const std::function<bool(const Partition_t &)> &function) const {
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

bool Builder::foreachForLogicalPartitions(const std::vector<std::string> &list,
                                          const std::function<bool(const Partition_t &)> &function) const {
  LOGI << "Foreaching input function for input list (only for logical partitions)." << std::endl;
  bool isSuccess = true;
  for (auto &name : list) {
    if (hasLogicalPartition(name)) isSuccess &= function(partition(name));
  }

  return isSuccess;
}

void Builder::reScan(bool auto_toggled) {
  LOGI << "Rescanning..." << std::endl;
  if (auto_toggled) {
    if (!buildAutoOnDiskChanges) return;
  }
  scan();
  scanLogicalPartitions();
}

void Builder::removeTable(const std::string &name) {
  LOGI << "Removing partition table (from list!): " << std::quoted(name) << std::endl;
  if (localTableNames.contains(name)) {
    localTableNames.erase(name);
    reScan(true);
  }
}

void Builder::setTables(std::unordered_set<std::string> names) {
  LOGI << "Setting up partition table list as input list." << std::endl;
  localTableNames = std::move(names);
}

void Builder::setGPTDataOf(const std::string &name, std::shared_ptr<GPTData> data) {
  LOGI << "Setting up GPTData of " << std::quoted(name) << " partition table." << std::endl;
  if (auto it = gptDataCollection.find("/dev/block/" + name); it != gptDataCollection.end()) it->second = std::move(data);
}

void Builder::setAutoScanOnTableChanges(bool state) { buildAutoOnDiskChanges = state; }

void Builder::clear() {
  LOGI << "Cleaning database." << std::endl;
  localPartitions.clear();
  localTableNames.clear();
  gptDataCollection.clear();
}

void Builder::reset() {
  LOGI << "Trigging clear() and resetting values to defaults." << std::endl;
  clear();
  buildAutoOnDiskChanges = true;
}

bool Builder::Extra::isReallyTable(const std::string &name) {
  std::filesystem::path p("/sys/class/block");
  p /= name;
  p /= "device";

  return std::filesystem::exists(p.string());
}

Builder::iterator Builder::begin() { return localPartitions.begin(); }
Builder::iterator Builder::end() { return localPartitions.end(); }

Builder::const_iterator Builder::begin() const { return localPartitions.begin(); }
Builder::const_iterator Builder::end() const { return localPartitions.end(); }
Builder::const_iterator Builder::cbegin() const { return localPartitions.cbegin(); }
Builder::const_iterator Builder::cend() const { return localPartitions.cend(); }

bool Builder::operator==(const Builder &other) const {
  bool equal = true;
  std::ranges::for_each(gptDataCollection, [&](auto &pair) {
    if (pair.second->GetDiskGUID() != other.GPTDataOf(pair.first)->GetDiskGUID()) equal = false;
  });

  return localPartitions == other.localPartitions && equal;
}

bool Builder::operator!=(const Builder &other) const { return !(*this == other); }

Builder::operator bool() const { return valid(); }

bool Builder::operator!() const { return !valid(); }

std::vector<const Partition_t *> Builder::operator*() const {
  std::vector<const Partition_t *> parts;
  parts.reserve(localPartitions.size());
  for (const auto &part : localPartitions)
    parts.push_back(&part);

  return parts;
}

std::vector<Partition_t *> Builder::operator*() {
  std::vector<Partition_t *> parts;
  parts.reserve(localPartitions.size());
  for (auto &part : localPartitions)
    parts.push_back(&part);

  return parts;
}

const std::shared_ptr<GPTData> &Builder::operator[](const std::string &name) const {
  const std::filesystem::path p("/dev/block/" + name);
  if (!gptDataCollection.contains(p)) throw ERR << "Can't find GPT data of " << name;
  return gptDataCollection.at(p);
}

std::shared_ptr<GPTData> &Builder::operator[](const std::string &name) {
  const std::filesystem::path p("/dev/block/" + name);
  if (!gptDataCollection.contains(p)) throw ERR << "Can't find GPT data of " << name;
  return gptDataCollection.at(p);
}

GPTPart *Builder::operator()(const std::string &name, uint32_t index) {
  if (!hasTable(name)) return nullptr;

  std::string name_;
  for (auto &part : localPartitions) {
    if (part.tableName() == name && part.index() == index) {
      name_ = part.name();
      break;
    }
  }

  return partition(name_).getGPTPartRef();
}

const GPTPart *Builder::operator()(const std::string &name, uint32_t index) const {
  if (!hasTable(name)) return nullptr;

  std::string name_;
  for (auto &part : localPartitions) {
    if (part.tableName() == name && part.index() == index) {
      name_ = part.name();
      break;
    }
  }

  return partition(name_).getGPTPartRef();
}

Builder &Builder::operator=(Builder &&other) noexcept {
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
