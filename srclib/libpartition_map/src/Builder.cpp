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
#include <libpartition_map/lib.hpp>
#include <ranges>
#include <utility>

namespace PartitionMap {

void Builder::scan() {
  if (diskNames.empty())
    throw Error("Empty disk path.");
  partitions.clear();
  gptDataCollection.clear();

  for (const auto &name : diskNames) {
    std::filesystem::path p("/dev/block");
    p /= name; // Append device.

    Helper::SilenceStdout silencer;
    auto gpt = std::make_shared<GPTData>();
    if (gpt->LoadPartitions(p)) {
      gptDataCollection[p] = std::move(gpt); // Add to GPT data list.

      auto &storedGpt = *gptDataCollection[p];
      for (uint32_t i = 0; i < storedGpt.GetNumParts(); ++i) {
        if (GPTPart part = storedGpt[i]; part.IsUsed())
          partitions.push_back(Partition_t({part, i, p})); // Add to partition list.
      }
    }
  }

  std::ranges::sort(
      partitions, [](Partition_t a, Partition_t b) { return a.getName() < b.getName(); });
}

void Builder::scanLogicalPartitions() {
  if (!Helper::directoryIsExists("/dev/block/mapper"))
    return;
  logicalPartitions.clear();

  std::vector<std::filesystem::directory_entry> entries{
      std::filesystem::directory_iterator("/dev/block/mapper"),
      std::filesystem::directory_iterator()};
  std::ranges::sort(entries, [](const auto &a, const auto &b) {
    return a.path().filename() < b.path().filename();
  });

  for (const auto &entry : entries) {
    std::filesystem::path final = std::filesystem::is_symlink(entry.path())
                                      ? std::filesystem::read_symlink(entry.path())
                                      : entry.path();
    if (std::filesystem::is_block_file(final))
      logicalPartitions.emplace_back(entry.path());
  }

  std::erase_if(logicalPartitions, [](const LogicalPartition_t &lpart) -> bool {
    return lpart.getName().find("com.") !=
           std::string::npos; // Erase non-partition contents (like
                              // com.android.adbd)
  });
}

void Builder::findDiskPaths() {
  try {
    std::vector<std::filesystem::directory_entry> entries{
        std::filesystem::directory_iterator("/dev/block"),
        std::filesystem::directory_iterator()};
    std::ranges::sort(entries, [](const auto &a, const auto &b) {
      return a.path().filename() < b.path().filename();
    });

    for (const auto &entry : entries) {
      if (std::filesystem::is_block_file(entry.status()) &&
          Extra::isReallyDisk(entry.path().filename()))
        diskNames.insert(entry.path().filename());
    }
  } catch (const std::filesystem::filesystem_error &e) {
    throw Error("%s", e.what());
  }

  if (diskNames.size() > 1)
    isUFS = true;
  if (diskNames.empty())
    throw Error("Can't find any disk or partition table in \"/dev/block\"");
  seek = *diskNames.begin();
}

std::vector<Partition_t> Builder::getPartitions() const { return partitions; }

std::vector<Partition_t> Builder::getPartitionsByDisk(const std::string &name) const {
  if (!diskNames.contains(name))
    return {};

  std::vector<Partition_t> parts;
  for (const auto &part : partitions)
    if (part.getDiskName() == name)
      parts.push_back(part);

  return parts;
}

std::vector<LogicalPartition_t> Builder::getLogicalPartitions() const {
  return logicalPartitions;
}

std::unordered_set<std::string> Builder::getDiskNames() const { return diskNames; }

std::unordered_set<std::filesystem::path> Builder::getDiskPaths() const {
  if (diskNames.empty())
    return {};

  std::unordered_set<std::filesystem::path> paths;
  for (const auto &name : diskNames)
    paths.insert("/dev/block/" + name);

  return paths;
}

const std::map<std::filesystem::path, std::shared_ptr<GPTData>> &
Builder::getAllGPTData() const {
  return gptDataCollection;
}

const std::shared_ptr<GPTData> &Builder::getGPTDataOf(const std::string &name) const {
  std::filesystem::path p("/dev/block");
  p /= name;
  if (!gptDataCollection.contains(p))
    throw Error("Can't find GPT data of %s", name.c_str());
  return gptDataCollection.at(p);
}

std::vector<std::pair<std::string, uint64_t>>
Builder::getDataOfLogicalPartitions() const {
  std::vector<std::pair<std::string, uint64_t>> parts;
  for (const auto &part : logicalPartitions)
    parts.emplace_back(part.getName(), part.getSize());

  return parts;
}

std::vector<std::tuple<std::string, uint64_t, bool>> Builder::getDataOfPartitions() {
  std::vector<std::tuple<std::string, uint64_t, bool>> parts;
  for (auto &part : partitions)
    parts.emplace_back(part.getName(), part.getSize(), part.isDynamic());

  return parts;
}

std::vector<std::tuple<std::string, uint64_t, bool>>
Builder::getDataOfPartitionsByDisk(const std::string &name) {
  std::vector<std::tuple<std::string, uint64_t, bool>> parts;
  for (auto &part : partitions)
    if (part.getDiskName() == name)
      parts.emplace_back(part.getName(), part.getSize(), part.isDynamic());

  return parts;
}

std::string Builder::getSeek() const { return seek; }

bool Builder::hasPartition(const std::string &name) {
  bool found = false;
  std::ranges::for_each(partitions, [&](auto &part) {
    if (part.getName() == name)
      found = true;
  });

  return found;
}

bool Builder::hasLogicalPartition(const std::string &name) const {
  bool found = false;
  std::ranges::for_each(logicalPartitions, [&](auto &part) {
    if (part.getName() == name)
      found = true;
  });

  return found;
}

bool Builder::hasDisk(const std::string &name) const { return diskNames.contains(name); }

bool Builder::isUsesUFS() const { return isUFS; }

bool Builder::isHasSuperPartition() const {
  return !logicalPartitions.empty() && std::filesystem::exists("/dev/block/mapper");
}

bool Builder::isLogical(const std::string &name) const {
  bool found = false;
  std::ranges::for_each(logicalPartitions, [&](auto &part) {
    if (part.getName() == name)
      found = true;
  });

  return found;
}

bool Builder::empty() const {
  return partitions.empty() && logicalPartitions.empty() && gptDataCollection.empty();
}

bool Builder::diskNamesEmpty() const { return diskNames.empty(); }

bool Builder::valid() {
  bool hasGptProblems = false;
  Helper::SilenceStdout silencer;
  std::ranges::for_each(gptDataCollection, [&](auto &pair) {
    if (pair.second->Verify() != 0 && pair.second->CheckHeaderValidity() != 3)
      hasGptProblems = true;
  });

  return !hasGptProblems;
}

bool Builder::foreachPartitions(const std::function<bool(Partition_t &)> &function) {
  bool isSuccess = true;
  for (auto &part : partitions)
    isSuccess &= function(part);

  return isSuccess;
}

bool Builder::foreachLogicalPartitions(
    const std::function<bool(LogicalPartition_t &)> &function) {
  bool isSuccess = true;
  for (auto &part : logicalPartitions)
    isSuccess &= function(part);

  return isSuccess;
}

bool Builder::foreachGptData(
    const std::function<bool(const std::filesystem::path &, std::shared_ptr<GPTData> &)>
        &function) {
  bool isSuccess = true;
  for (auto &[path, gptData] : gptDataCollection)
    isSuccess &= function(path, gptData);

  return isSuccess;
}

void Builder::reScan(bool auto_toggle) {
  if (auto_toggle) {
    if (!buildAutoOnDiskChanges)
      return;
  }
  scan();
  scanLogicalPartitions();
}

void Builder::addDisk(const std::string &name) {
  if (!diskNames.contains(name)) {
    diskNames.insert(name);
    reScan(true);
  }
}

void Builder::removeDisk(const std::string &name) {
  if (diskNames.contains(name)) {
    diskNames.erase(name);
    reScan(true);
  }
}

void Builder::setSeek(const std::string &name) {
  if (diskNames.contains(name))
    seek = name;
}

void Builder::setDisks(std::unordered_set<std::string> names) {
  diskNames = std::move(names);
}

void Builder::setGPTDataOf(const std::string &name, std::shared_ptr<GPTData> data) {
  if (auto it = gptDataCollection.find("/dev/block/" + name);
      it != gptDataCollection.end()) {
    it->second = std::move(data);
  }
}

void Builder::setAutoScanOnDiskChanges(bool state) { buildAutoOnDiskChanges = state; }

void Builder::clear() {
  partitions.clear();
  logicalPartitions.clear();
  diskNames.clear();
  gptDataCollection.clear();
}

void Builder::reset() {
  clear();
  buildAutoOnDiskChanges = true;
  seek = diskNames.empty() ? "" : *diskNames.begin();
}

bool Builder::Extra::isReallyDisk(const std::string &name) {
  std::filesystem::path p("/sys/class/block");
  p /= name;
  p /= "device";

  return std::filesystem::exists(p.string());
}

bool Builder::operator==(const Builder &other) {
  bool equal = true;
  std::ranges::for_each(gptDataCollection, [&](auto &pair) {
    if (pair.second->GetDiskGUID() != other.getGPTDataOf(pair.first)->GetDiskGUID())
      equal = false;
  });

  return partitions == other.partitions && logicalPartitions == other.logicalPartitions &&
         equal;
}

bool Builder::operator!=(const Builder &other) { return !(*this == other); }

Builder::operator bool() { return valid(); }

bool Builder::operator!() { return !valid(); }

const std::shared_ptr<GPTData> &Builder::operator[](const std::string &name) const {
  std::filesystem::path p("/dev/block/" + name);
  if (!gptDataCollection.contains(p))
    throw Error("Can't find GPT data of %s", name.c_str());
  return gptDataCollection.at(p);
}

GPTPart Builder::operator[](uint32_t index) {
  if (!hasDisk(seek))
    return {};

  GPTPart gptPart;
  for (auto &part : partitions) {
    if (part.getDiskName() == seek && part.getIndex() == index) {
      gptPart = part.getGPTPart();
      break;
    }
  }

  return gptPart;
}

} // namespace PartitionMap
