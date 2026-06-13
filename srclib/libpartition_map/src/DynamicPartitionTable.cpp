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
#include <libhelper/functions.hpp>
#include <libpartition_map/table_data_collection.hpp>
#include <libpartition_map/definations.hpp>
#include <libpartition_map/redefine_logging_macros.hpp>
#include <liblp/liblp.h>

using namespace android;

namespace PartitionMap {

void DynamicTableData::scan() {
  if (!Helper::isExists("/dev/block/by-name/super")) {
    LOGI << "This device not uses logical partitions." << std::endl;
    return;
  } else
    supported = true;

  LOGI << "Scanning super metadata and partitions with liblp..." << std::endl;
  lpMetadata = std::move(fs_mgr::ReadMetadata("/dev/block/by-name/super", 0));

  for (const auto &partition : lpMetadata->partitions) {
    Partition_t part;
    localPartitions.push_back(std::move(Partition_t::AsLogicalPartition(part, Helper::pathJoin("/dev/block/mapper", partition.name))));
    LOGI << "Registered logical partition: " << partition.name << std::endl;
  }
}

DynamicTableData::list_t DynamicTableData::partitions() {
  LOGI << "Providing references of logical partitions." << std::endl;
  list_t parts;
  for (auto &part : localPartitions)
    parts.push_back(std::ref(part));

  return parts;
}

DynamicTableData::const_list_t DynamicTableData::partitions() const {
  LOGI << "Providing references of logical partitions." << std::endl;
  const_list_t parts;
  for (auto &part : localPartitions)
    parts.push_back(std::cref(part));

  return parts;
}

android::fs_mgr::LpMetadata &DynamicTableData::getMetadata() {
  LOGI << "Providing super metadata." << std::endl;
  return *lpMetadata;
}

const android::fs_mgr::LpMetadata &DynamicTableData::getMetadata() const {
  LOGI << "Providing super metadata." << std::endl;
  return *lpMetadata;
}

std::vector<LpMetadataPartitionGroup> &DynamicTableData::getGroups() { return lpMetadata->groups; }

const std::vector<LpMetadataPartitionGroup> &DynamicTableData::getGroups() const { return lpMetadata->groups; }

std::vector<BasicInfo> DynamicTableData::aboutPartitions() const {
  LOGI << "Providing data of logical partitions." << std::endl;
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

  LOGI << "Providing Partition_t object of " << std::quoted(name) << " logical partition." << std::endl;
  return std::ref(*it);
}

std::optional<std::reference_wrapper<const Partition_t>> DynamicTableData::partition(const std::string &name,
                                                                                     const std::string &) const {
  auto it = std::ranges::find_if(localPartitions, [&](const Partition_t &p) { return p.name() == name; });
  if (it == localPartitions.end()) return std::nullopt;

  LOGI << "Providing Partition_t object of " << std::quoted(name) << " logical partition." << std::endl;
  return std::cref(*it);
}

std::optional<std::reference_wrapper<LpMetadataPartition>> DynamicTableData::metadata(const std::string &name) {
  auto it = std::ranges::find_if(lpMetadata->partitions, [&](const LpMetadataPartition &p) { return name == p.name; });
  if (it == lpMetadata->partitions.end()) return std::nullopt;

  LOGI << "Providing LpMetadataPartition object of " << std::quoted(name) << " logical partition." << std::endl;
  return std::ref(*it);
}

std::optional<std::reference_wrapper<const LpMetadataPartition>> DynamicTableData::metadata(const std::string &name) const {
  auto it = std::ranges::find_if(lpMetadata->partitions, [&](const LpMetadataPartition &p) { return name == p.name; });
  if (it == lpMetadata->partitions.end()) return std::nullopt;

  LOGI << "Providing LpMetadataPartition object of " << std::quoted(name) << " logical partition." << std::endl;
  return std::cref(*it);
}

uint64_t DynamicTableData::freeSpace() const {
  LOGI << "Providing free space of super partition." << std::endl;
  uint64_t total_part_sizes = 0;

  for (const auto &p : localPartitions)
    total_part_sizes += p.size();

  return lpMetadata->block_devices[0].size - total_part_sizes;
}

uint64_t DynamicTableData::freeSpace(const std::string &name) const {
  LOGI << "Providing free space of " << std::quoted(name) << " group." << std::endl;
  auto it = std::ranges::find_if(lpMetadata->groups, [&](const LpMetadataPartitionGroup &group) { return name == group.name; });

  if (it == lpMetadata->groups.end()) return UINT64_MAX;

  uint64_t used_size = 0;
  for (const auto &p : lpMetadata->partitions) {
    if (lpMetadata->groups[p.group_index].name == name) used_size += partition(name)->get().size();
  }

  return (*it).maximum_size - used_size;
}

uint64_t DynamicTableData::size() const {
  LOGI << "Providing size of super partition." << std::endl;
  return lpMetadata->block_devices[0].size;
}

uint64_t DynamicTableData::size(const std::string &name) {
  LOGI << "Providing maximum size of " << std::quoted(name) << " group." << std::endl;
  auto it = std::ranges::find_if(lpMetadata->groups, [&](const LpMetadataPartitionGroup &group) { return name == group.name; });

  if (it == lpMetadata->groups.end()) return UINT64_MAX;
  return (*it).maximum_size;
}

bool DynamicTableData::hasPartition(const std::string &name) const {
  LOGI << "Checking " << std::quoted(name) << " named logical partition is exists." << std::endl;
  bool found = false;
  std::ranges::for_each(localPartitions, [&](auto &part) {
    if (part.name() == name) found = true;
  });

  return found;
}

bool DynamicTableData::empty() const {
  LOGI << "Checking whether the logical partition list is empty." << std::endl;
  return localPartitions.empty();
}

bool DynamicTableData::valid() const {
  LOGI << "Checking whether the logical partition list and metadata is valid." << std::endl;
  return !localPartitions.empty() && validMetadata();
}

bool DynamicTableData::validMetadata() const {
  LOGI << "Checking whether the metadata is valid." << std::endl;
  if (static_cast<bool>(lpMetadata)) {
    if (lpMetadata->header.magic != LP_METADATA_HEADER_MAGIC) {
      LOGE << "Super partition is not contains " << LP_METADATA_HEADER_MAGIC << " magic." << std::endl;
      return false;
    }

    for (const auto &group : lpMetadata->groups) {
      if (group.maximum_size == 0) continue;

      uint64_t total = 0;
      for (const auto &p : lpMetadata->partitions) {
        if (p.group_index == &group - &lpMetadata->groups[0]) total += partition(p.name)->get().size();
      }

      if (total > group.maximum_size) {
        LOGE << "Group limits have been exceeded." << std::endl;
        return false;
      }
    }

    for (const auto &extent : lpMetadata->extents) {
      if (extent.target_type != LP_TARGET_TYPE_LINEAR) continue;

      const auto &bd = lpMetadata->block_devices[extent.target_source];
      uint64_t end = (extent.target_data + extent.num_sectors) * 512;

      if (end > bd.size) {
        LOGE << "Extent Block Device limit has been exceeded." << std::endl;
        return false;
      }
    }

    for (const auto &partition : lpMetadata->partitions) {
      uint32_t max_index = partition.first_extent_index + partition.num_extents;
      if (max_index > lpMetadata->extents.size()) {
        LOGE << "The number of partition extents is inconsistent." << std::endl;
        return false;
      }
    }

    return true;
  }

  return false;
}

void DynamicTableData::reScan() {
  LOGI << "Rescanning logical partitions." << std::endl;
  localPartitions.clear();
  scan();
}

void DynamicTableData::clear() {
  LOGI << "Clearing data." << std::endl;
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
