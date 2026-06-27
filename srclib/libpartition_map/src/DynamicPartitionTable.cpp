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
#include <ranges>
#include <utility>
#include <libhelper/functions.hpp>
#include <libpartition_map/table_data_collection.hpp>
#include <libpartition_map/definations.hpp>
#include <liblp/liblp.h>

using namespace android;

namespace PartitionMap {

void DynamicTableData::scan() {
  if (!Helper::isExists("/dev/block/by-name/super")) {
    Log::info("This device not uses logical partitions.");
    return;
  } else
    supported = true;

  Log::info("Scanning super metadata and partitions with liblp...");
  lpMetadata = std::move(fs_mgr::ReadMetadata("/dev/block/by-name/super", 0));

  for (const auto &partition : lpMetadata->partitions) {
    Partition_t part;
    localPartitions.push_back(std::move(Partition_t::AsLogicalPartition(part, Helper::pathJoin("/dev/block/mapper", partition.name))));
    Log::info("Registered logical partition: {}", std::quoted_string(partition.name));
  }
}

DynamicTableData::list_t DynamicTableData::partitions() {
  Log::info("Providing references of logical partitions.");
  list_t parts;
  for (auto &part : localPartitions)
    parts.push_back(std::ref(part));

  return parts;
}

DynamicTableData::const_list_t DynamicTableData::partitions() const {
  Log::info("Providing references of logical partitions.");
  const_list_t parts;
  for (auto &part : localPartitions)
    parts.push_back(std::cref(part));

  return parts;
}

android::fs_mgr::LpMetadata &DynamicTableData::getMetadata() {
  Log::info("Providing super metadata.");
  return *lpMetadata;
}

const android::fs_mgr::LpMetadata &DynamicTableData::getMetadata() const {
  Log::info("Providing super metadata.");
  return *lpMetadata;
}

std::vector<LpMetadataPartitionGroup> &DynamicTableData::getGroups() { return lpMetadata->groups; }

const std::vector<LpMetadataPartitionGroup> &DynamicTableData::getGroups() const { return lpMetadata->groups; }

std::vector<BasicInfo> DynamicTableData::aboutPartitions() const {
  Log::info("Providing data of logical partitions.");
  std::vector<BasicInfo> parts;
  for (const auto &p : partitions()) {
    const Partition_t &part = p;
    parts.emplace_back(part.name(), part.size(), false);
  }

  return parts;
}

std::optional<std::reference_wrapper<Partition_t>> DynamicTableData::partition(const std::string &name, const std::string &) {
  auto it = std::ranges::find_if(localPartitions, [&](const Partition_t &p) { return p.name() == name; });
  if (it == localPartitions.end()) return std::nullopt;

  Log::info("Providing Partition_t object of {} logical partition.", std::quoted_string(name));
  return std::ref(*it);
}

std::optional<std::reference_wrapper<const Partition_t>> DynamicTableData::partition(const std::string &name,
                                                                                     const std::string &) const {
  auto it = std::ranges::find_if(localPartitions, [&](const Partition_t &p) { return p.name() == name; });
  if (it == localPartitions.end()) return std::nullopt;

  Log::info("Providing Partition_t object of {} logical partition.", std::quoted_string(name));
  return std::cref(*it);
}

std::optional<std::reference_wrapper<LpMetadataPartition>> DynamicTableData::metadata(const std::string &name) {
  auto it = std::ranges::find_if(lpMetadata->partitions, [&](const LpMetadataPartition &p) { return name == p.name; });
  if (it == lpMetadata->partitions.end()) return std::nullopt;

  Log::info("Providing LpMetadataPartition object of {} logical partition.", std::quoted_string(name));
  return std::ref(*it);
}

std::optional<std::reference_wrapper<const LpMetadataPartition>> DynamicTableData::metadata(const std::string &name) const {
  auto it = std::ranges::find_if(lpMetadata->partitions, [&](const LpMetadataPartition &p) { return name == p.name; });
  if (it == lpMetadata->partitions.end()) return std::nullopt;

  Log::info("Providing LpMetadataPartition object of {} logical partition.", std::quoted_string(name));
  return std::cref(*it);
}

uint64_t DynamicTableData::freeSpace() const {
  Log::info("Providing free space of super partition.");
  uint64_t total_part_sizes = 0;

  for (const auto &p : localPartitions)
    total_part_sizes += p.size();

  return lpMetadata->block_devices[0].size - total_part_sizes;
}

uint64_t DynamicTableData::freeSpace(const std::string &name) const {
  Log::info("Providing free space of {} group.", std::quoted_string(name));
  auto it = std::ranges::find_if(lpMetadata->groups, [&](const LpMetadataPartitionGroup &group) { return name == group.name; });

  if (it == lpMetadata->groups.end()) return UINT64_MAX;

  uint64_t used_size = 0;
  for (const auto &p : lpMetadata->partitions) {
    if (lpMetadata->groups[p.group_index].name == name) used_size += partition(name)->get().size();
  }

  return (*it).maximum_size - used_size;
}

uint64_t DynamicTableData::size() const {
  Log::info("Providing size of super partition.");
  return lpMetadata->block_devices[0].size;
}

uint64_t DynamicTableData::size(const std::string &name) {
  Log::info("Providing maximum size of {} group.", std::quoted_string(name));
  auto it = std::ranges::find_if(lpMetadata->groups, [&](const LpMetadataPartitionGroup &group) { return name == group.name; });

  if (it == lpMetadata->groups.end()) return UINT64_MAX;
  return (*it).maximum_size;
}

bool DynamicTableData::hasPartition(const std::string &name) const {
  Log::info("Checking {} named logical partition is exists.", std::quoted_string(name));
  bool found = false;
  std::ranges::for_each(localPartitions, [&](auto &part) {
    if (part.name() == name) found = true;
  });

  return found;
}

bool DynamicTableData::empty() const {
  Log::info("Checking whether the logical partition list is empty.");
  return localPartitions.empty();
}

bool DynamicTableData::valid() const {
  Log::info("Checking whether the logical partition list and metadata is valid.");
  return !localPartitions.empty() && validMetadata();
}

bool DynamicTableData::validMetadata() const {
  Log::info("Checking whether the metadata is valid.");
  if (static_cast<bool>(lpMetadata)) {
    if (lpMetadata->header.magic != LP_METADATA_HEADER_MAGIC) {
      Log::error("Super partition is not contains {:#x} magic.", LP_METADATA_HEADER_MAGIC);
      return false;
    }

    for (const auto &group : lpMetadata->groups) {
      if (group.maximum_size == 0) continue;

      uint64_t total = 0;
      for (const auto &p : lpMetadata->partitions) {
        if (p.group_index == &group - &lpMetadata->groups[0]) total += partition(p.name)->get().size();
      }

      if (total > group.maximum_size) {
        Log::error("Group limits have been exceeded.");
        return false;
      }
    }

    for (const auto &extent : lpMetadata->extents) {
      if (extent.target_type != LP_TARGET_TYPE_LINEAR) continue;

      const auto &bd = lpMetadata->block_devices[extent.target_source];
      uint64_t end = (extent.target_data + extent.num_sectors) * 512;

      if (end > bd.size) {
        Log::error("Extent Block Device limit has been exceeded.");
        return false;
      }
    }

    for (const auto &partition : lpMetadata->partitions) {
      uint32_t max_index = partition.first_extent_index + partition.num_extents;
      if (max_index > lpMetadata->extents.size()) {
        Log::error("The number of partition extents is inconsistent.");
        return false;
      }
    }

    return true;
  }

  return false;
}

void DynamicTableData::reScan() {
  Log::info("Rescanning logical partitions.");
  localPartitions.clear();
  scan();
}

void DynamicTableData::clear() {
  Log::info("Clearing data.");
  localPartitions.clear();
  lpMetadata.reset();
}

void DynamicTableData::reset() { clear(); }

DynamicTableData::iterator DynamicTableData::begin() { return localPartitions.begin(); }
DynamicTableData::iterator DynamicTableData::end() { return localPartitions.end(); }

DynamicTableData::const_iterator DynamicTableData::begin() const { return localPartitions.begin(); }
DynamicTableData::const_iterator DynamicTableData::end() const { return localPartitions.end(); }
DynamicTableData::const_iterator DynamicTableData::cbegin() const { return localPartitions.cbegin(); }
DynamicTableData::const_iterator DynamicTableData::cend() const { return localPartitions.cend(); }

bool DynamicTableData::operator==(const DynamicTableData &other) const {
  return localPartitions == other.localPartitions && lpMetadata == other.lpMetadata;
}

bool DynamicTableData::operator!=(const DynamicTableData &other) const { return !(*this == other); }

DynamicTableData::operator bool() const { return valid(); }

bool DynamicTableData::operator!() const { return !valid(); }

DynamicTableData::const_list_t DynamicTableData::operator*() const {
  const_list_t parts;
  parts.reserve(localPartitions.size());
  for (const auto &part : localPartitions)
    parts.push_back(std::cref(part));

  return parts;
}

DynamicTableData::list_t DynamicTableData::operator*() {
  list_t parts;
  parts.reserve(localPartitions.size());
  for (auto &part : localPartitions)
    parts.push_back(std::ref(part));

  return parts;
}

DynamicTableData &DynamicTableData::operator=(const DynamicTableData &other) {
  if (this != &other) {
    localPartitions = other.localPartitions;
    lpMetadata = std::make_unique<fs_mgr::LpMetadata>(*(other.lpMetadata));
  }

  return *this;
}

DynamicTableData &DynamicTableData::operator=(DynamicTableData &&other) noexcept {
  if (this != &other) {
    localPartitions = std::move(other.localPartitions);
    lpMetadata = std::move(other.lpMetadata);
  }

  return *this;
}

} // namespace PartitionMap
