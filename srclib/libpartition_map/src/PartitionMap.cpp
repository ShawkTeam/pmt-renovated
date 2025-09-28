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

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#ifndef ANDROID_BUILD
#include <generated/buildInfo.hpp>
#endif
#include <iostream>
#include <libpartition_map/lib.hpp>
#include <linux/fs.h>
#include <memory>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

static constexpr std::array<std::string_view, 3> defaultEntryList = {
    "/dev/block/by-name", "/dev/block/bootdevice/by-name",
    "/dev/block/platform/bootdevice/by-name"};

namespace PartitionMap {
bool basic_partition_map_builder::_is_real_block_dir(
    const std::string_view path) {
  if (path.find("/block/") == std::string::npos) {
    LOGN(MAP, ERROR) << "Path " << path << " is not a real block directory.";
    return false;
  }
  return true;
}

Map_t basic_partition_map_builder::_build_map(std::string_view path,
                                              const bool logical) {
  if (!Helper::directoryIsExists(path) && logical) {
    LOGN(MAP, WARNING) << "This device not contains logical partitions."
                       << std::endl;
    return {};
  }

  Map_t map;
  std::vector<std::filesystem::directory_entry> entries{
      std::filesystem::directory_iterator(path),
      std::filesystem::directory_iterator()};
  std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
    return a.path().filename() < b.path().filename();
  });

  LOGN_IF(MAP, WARNING, entries.empty())
      << path
      << " is exists but generated vector is empty "
         "(std::vector<std::filesystem::directory_entry>)."
      << std::endl;
  for (const auto &entry : entries) {
    if (entry.path().filename() != "by-uuid" &&
        std::string(entry.path()).find("com.") == std::string::npos)
      map.insert(entry.path().filename().string(), _get_size(entry.path()),
                 logical);
  }

  LOGN(MAP, INFO) << std::boolalpha
                  << "Map generated successfully. is_logical_map=" << logical
                  << std::endl;
  return map;
}

void basic_partition_map_builder::_insert_logicals(Map_t &&logicals) {
  LOGN(MAP, INFO)
      << "merging created logical partition list to this object's variable."
      << std::endl;
  _current_map.merge(logicals);
  LOGN(MAP, INFO) << "Cleaning created logical partition because not need more."
                  << std::endl;
  logicals.clear();
}

void basic_partition_map_builder::_map_build_check() const {
  if (!_map_builded) throw Error("Please build partition map before!");
}

uint64_t basic_partition_map_builder::_get_size(const std::string &path) {
  const std::string real = std::filesystem::read_symlink(path);
  Helper::garbageCollector collector;

  const int fd = Helper::openAndAddToCloseList(real, collector, O_RDONLY);
  if (fd < 0) {
    LOGN(MAP, ERROR) << "Cannot open " << real << ": " << strerror(errno)
                     << std::endl;
    return 0;
  }

  uint64_t size = 0;
  if (ioctl(fd, BLKGETSIZE64, &size) != 0) {
    LOGN(MAP, ERROR) << "ioctl() process failed for " << real << ": "
                     << strerror(errno) << std::endl;
    return 0;
  }

  return size;
}

basic_partition_map_builder::basic_partition_map_builder() {
  LOGN(MAP, INFO) << "default constructor called. Starting build." << std::endl;

  for (const auto &path : defaultEntryList) {
    if (std::filesystem::exists(path)) {
      _current_map = _build_map(path);
      if (_current_map.empty()) {
        _any_generating_error = true;
      } else {
        _workdir = path;
        break;
      }
    }
  }

  if (_current_map.empty())
    LOGN(MAP, ERROR) << "Cannot build map by any default search entry."
                     << std::endl;

  LOGN(MAP, INFO) << "default constructor ended work." << std::endl;
  _insert_logicals(_build_map("/dev/block/mapper", true));
  _map_builded = true;
}

basic_partition_map_builder::basic_partition_map_builder(
    const std::string_view path) {
  LOGN(MAP, INFO) << "argument-based constructor called. Starting build."
                  << std::endl;

  if (std::filesystem::exists(path)) {
    if (!_is_real_block_dir(path)) return;
    _current_map = _build_map(path);
    if (_current_map.empty()) _any_generating_error = true;
    else _workdir = path;
  } else
    throw Error("Cannot find directory: %s. Cannot build partition map!",
                path.data());

  LOGN(MAP, INFO) << "argument-based constructor successfully ended work."
                  << std::endl;
  _insert_logicals(_build_map("/dev/block/mapper", true));
  _map_builded = true;
}

basic_partition_map_builder::basic_partition_map_builder(
    basic_partition_map_builder &&other) noexcept {
  _current_map = Map_t(std::move(other._current_map));
  _workdir = std::move(other._workdir);
  _any_generating_error = other._any_generating_error;
  _map_builded = other._map_builded;
  other.clear();
}

bool basic_partition_map_builder::hasPartition(
    const std::string_view name) const {
  _map_build_check();
  return _current_map.find(name);
}

bool basic_partition_map_builder::hasLogicalPartitions() const {
  _map_build_check();
  for (const auto &[name, props] : _current_map)
    if (props.isLogical) return true;

  return false;
}

bool basic_partition_map_builder::isLogical(const std::string_view name) const {
  _map_build_check();
  return _current_map.is_logical(name);
}

void basic_partition_map_builder::clear() {
  _current_map.clear();
  _workdir.clear();
  _any_generating_error = false;
}

bool basic_partition_map_builder::readDirectory(const std::string_view path) {
  _map_builded = false;
  LOGN(MAP, INFO) << "read " << path << " directory request." << std::endl;

  if (std::filesystem::exists(path)) {
    if (!_is_real_block_dir(path)) return false;
    _current_map = _build_map(path);
    if (_current_map.empty()) {
      _any_generating_error = true;
      return false;
    } else _workdir = path;
  } else
    throw Error("Cannot find directory: %s. Cannot build partition map!",
                path.data());

  LOGN(MAP, INFO) << "read " << path << " successfully." << std::endl;
  _insert_logicals(_build_map("/dev/block/mapper", true));
  _map_builded = true;
  return true;
}

bool basic_partition_map_builder::readDefaultDirectories() {
  _map_builded = false;
  LOGN(MAP, INFO) << "read default directories request." << std::endl;

  for (const auto &path : defaultEntryList) {
    if (std::filesystem::exists(path)) {
      _current_map = _build_map(path);
      if (_current_map.empty()) {
        _any_generating_error = true;
        return false;
      } else {
        _workdir = path;
        break;
      }
    }
  }

  if (_current_map.empty())
    LOGN(MAP, ERROR) << "Cannot build map by any default search entry."
                     << std::endl;

  LOGN(MAP, INFO) << "read default directories successfully." << std::endl;
  _insert_logicals(_build_map("/dev/block/mapper", true));
  _map_builded = true;
  return true;
}

bool basic_partition_map_builder::copyPartitionsToVector(
    std::vector<std::string> &vec) const {
  if (_current_map.empty()) {
    LOGN(MAP, ERROR) << "Current map is empty.";
    return false;
  }
  vec.clear();
  for (const auto &[name, props] : _current_map)
    vec.push_back(name);
  return true;
}

bool basic_partition_map_builder::copyLogicalPartitionsToVector(
    std::vector<std::string> &vec) const {
  if (_current_map.empty()) {
    LOGN(MAP, ERROR) << "Current map is empty.";
    return false;
  }
  std::vector<std::string> vec2;
  for (const auto &[name, props] : _current_map)
    if (props.isLogical) vec2.push_back(name);

  if (vec2.empty()) {
    LOGN(MAP, ERROR) << "Cannot find logical partitions in current map.";
    return false;
  } else vec = vec2;
  return true;
}

bool basic_partition_map_builder::copyPhysicalPartitionsToVector(
    std::vector<std::string> &vec) const {
  if (_current_map.empty()) {
    LOGN(MAP, ERROR) << "Current map is empty.";
    return false;
  }
  std::vector<std::string> vec2;
  for (const auto &[name, props] : _current_map)
    if (!props.isLogical) vec2.push_back(name);

  if (vec2.empty()) {
    LOGN(MAP, ERROR) << "Cannot find physical partitions in current map.";
    return false;
  } else vec = vec2;
  return true;
}

bool basic_partition_map_builder::empty() const {
  _map_build_check();
  return _current_map.empty();
}

bool basic_partition_map_builder::doForAllPartitions(
    const std::function<bool(std::string, BasicInf)> &func) const {
  _map_build_check();
  bool err = false;

  LOGN(MAP, INFO) << "Doing input function for all partitions." << std::endl;
  for (const auto &[name, props] : _current_map) {
    if (func(name, {props.size, props.isLogical}))
      LOGN(MAP, INFO) << "Done progress for " << name << " partition."
                      << std::endl;
    else {
      err = true;
      LOGN(MAP, ERROR) << "Failed progress for " << name << " partition."
                       << std::endl;
    }
  }

  return err;
}

bool basic_partition_map_builder::doForPhysicalPartitions(
    const std::function<bool(std::string, BasicInf)> &func) const {
  _map_build_check();
  bool err = false;

  LOGN(MAP, INFO) << "Doing input function for physical partitions."
                  << std::endl;
  for (const auto &[name, props] : _current_map) {
    if (props.isLogical) continue;
    if (func(name, {props.size, props.isLogical}))
      LOGN(MAP, INFO) << "Done progress for " << name << " partition."
                      << std::endl;
    else {
      err = true;
      LOGN(MAP, ERROR) << "Failed progress for " << name << " partition."
                       << std::endl;
    }
  }

  return err;
}

bool basic_partition_map_builder::doForLogicalPartitions(
    const std::function<bool(std::string, BasicInf)> &func) const {
  _map_build_check();
  bool err = false;

  LOGN(MAP, INFO) << "Doing input function for logical partitions."
                  << std::endl;
  for (const auto &[name, props] : _current_map) {
    if (!props.isLogical) continue;
    if (func(name, {props.size, props.isLogical}))
      LOGN(MAP, INFO) << "Done progress for " << name << " partition."
                      << std::endl;
    else {
      err = true;
      LOGN(MAP, ERROR) << "Failed progress for " << name << " partition."
                       << std::endl;
    }
  }

  return err;
}

bool basic_partition_map_builder::doForPartitionList(
    const std::vector<std::string> &partitions,
    const std::function<bool(std::string, BasicInf)> &func) const {
  _map_build_check();
  bool err = false;

  LOGN(MAP, INFO) << "Doing input function for input partition list."
                  << std::endl;
  for (const auto &partition : partitions) {
    if (!hasPartition(partition))
      throw Error("Couldn't find partition: %s", partition.data());
    if (!func(partition, _current_map[partition])) {
      err = true;
      LOGN(MAP, ERROR) << "Failed progress for " << partition << " partition."
                       << std::endl;
    }
  }

  return err;
}

uint64_t
basic_partition_map_builder::sizeOf(const std::string_view name) const {
  _map_build_check();
  return _current_map.get_size(name);
}

bool operator==(const basic_partition_map_builder &lhs,
                const basic_partition_map_builder &rhs) {
  return lhs._current_map == rhs._current_map;
}

bool operator!=(const basic_partition_map_builder &lhs,
                const basic_partition_map_builder &rhs) {
  return !(lhs == rhs);
}

basic_partition_map_builder::operator bool() const {
  return !this->_any_generating_error;
}

bool basic_partition_map_builder::operator!() const {
  return this->_any_generating_error;
}

bool basic_partition_map_builder::operator()(const std::string_view path) {
  LOGN(MAP, INFO) << "calling readDirectory() for building map with " << path
                  << std::endl;
  return readDirectory(path);
}

Map_t &basic_partition_map_builder::operator*() { return _current_map; }

const Map_t &basic_partition_map_builder::operator*() const {
  return _current_map;
}

Info basic_partition_map_builder::operator[](const int index) const {
  return _current_map[index];
}

BasicInf
basic_partition_map_builder::operator[](const std::string_view &name) const {
  return _current_map[name];
}

basic_partition_map_builder::operator std::vector<Info>() const {
  return static_cast<std::vector<Info>>(_current_map);
}

basic_partition_map_builder::operator int() const {
  return static_cast<int>(_current_map);
}

basic_partition_map_builder::operator std::string() const { return _workdir; }

std::string getLibVersion() { MKVERSION("libpartition_map"); }
} // namespace PartitionMap
