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

#include <filesystem>
#include <libpartition_map/lib.hpp>
#include <optional>
#include <string>

namespace PartitionMap {
Map_t basic_partition_map_builder::getAll() const {
  _map_build_check();
  return _current_map;
}

std::optional<std::pair<uint64_t, bool>>
basic_partition_map_builder::get(const std::string_view name) const {
  _map_build_check();

  if (!_current_map.find(name)) return std::nullopt;
  return std::make_pair(_current_map.get_size(name),
                        _current_map.is_logical(name));
}

std::optional<std::list<std::string>>
basic_partition_map_builder::getLogicalPartitionList() const {
  _map_build_check();

  std::list<std::string> logicals;
  for (const auto &[name, props] : _current_map)
    if (props.isLogical) logicals.push_back(name);

  if (logicals.empty()) return std::nullopt;
  return logicals;
}

std::optional<std::list<std::string>>
basic_partition_map_builder::getPhysicalPartitionList() const {
  _map_build_check();

  std::list<std::string> physicals;
  for (const auto &[name, props] : _current_map)
    if (!props.isLogical) physicals.push_back(name);

  if (physicals.empty()) return std::nullopt;
  return physicals;
}

std::optional<std::list<std::string>>
basic_partition_map_builder::getPartitionList() const {
  _map_build_check();

  std::list<std::string> partitions;
  for (const auto &[name, props] : _current_map)
    partitions.push_back(name);

  if (partitions.empty()) return std::nullopt;
  return partitions;
}

std::string basic_partition_map_builder::getRealLinkPathOf(
    const std::string_view name) const {
  _map_build_check();

  if (!_current_map.find(name)) return {};
  return std::string(_workdir + "/" + name.data());
}

std::string
basic_partition_map_builder::getRealPathOf(const std::string_view name) const {
  _map_build_check();

  const std::string full = (isLogical(name))
                               ? std::string("/dev/block/mapper/") + name.data()
                               : _workdir + "/" + name.data();
  if (!_current_map.find(name) || !std::filesystem::is_symlink(full)) return {};

  return std::filesystem::read_symlink(full);
}

std::string basic_partition_map_builder::getCurrentWorkDir() const {
  return _workdir;
}
} // namespace PartitionMap
