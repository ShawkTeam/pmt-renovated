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

#include <asm-generic/fcntl.h>
#include <filesystem>
#include <libpartition_map/lib.hpp>
#include <libpartition_map/redefine_logging_macros.hpp>
#include <ostream>

namespace PartitionMap {

bool Partition_t::Extra::isReallyLogical(const std::filesystem::path &path) {
  return path.string().find("/mapper/") != std::string::npos;
}

Partition_t &Partition_t::AsLogicalPartition(Partition_t &orig, const std::filesystem::path &path) {
  orig.isLogical = true;
  orig.logicalPartitionPath = path;
  orig.gptPart = GPTPart();
  return orig;
}

GPTPart Partition_t::getGPTPart() {
  if (isLogical) throw Error("This is not a normal partition object!");
  return gptPart;
}

GPTPart *Partition_t::getGPTPartRef() {
  if (isLogical) throw Error("This is not a normal partition object!");
  return &gptPart;
}

std::filesystem::path Partition_t::getPath() const {
  std::string suffix = isdigit(tablePath.string().back()) ? "p" : "";
  std::string path = isLogical ? logicalPartitionPath : std::filesystem::path(tablePath.string() + suffix + std::to_string(index + 1));
  return path;
}

std::filesystem::path Partition_t::getAbsolutePath() const {
  if (isLogical) return std::filesystem::read_symlink(logicalPartitionPath);
  return getPath();
}

std::filesystem::path Partition_t::getDiskPath() const {
  if (isLogical) throw Error("This is not a normal partition object!");
  return tablePath;
}

std::filesystem::path Partition_t::getPathByName() {
  if (isLogical) return logicalPartitionPath;
  std::filesystem::path result = "/dev/block/by-name";
  result /= gptPart.GetDescription();

  if (!std::filesystem::exists("/dev/block/by-name") || std::filesystem::read_symlink(result) != getPath()) return {};
  return result;
}

std::string Partition_t::getName() {
  if (isLogical) return logicalPartitionPath.filename().string();
  return gptPart.GetDescription();
}

std::string Partition_t::getTableName() const {
  if (isLogical) throw Error("This is not a normal partition object!");
  return tablePath.filename();
}

std::string Partition_t::getFormattedSizeString(SizeUnit size_unit, bool no_type) const {
  uint64_t size = getSize();
  switch (size_unit) {
    case BYTE:
      return no_type ? std::to_string(size) : std::to_string(size) + "B";
    case KiB:
      return no_type ? std::to_string(TO_KB(size)) : std::to_string(TO_KB(size)) + "KiB";
    case MiB:
      return no_type ? std::to_string(TO_MB(size)) : std::to_string(TO_MB(size)) + "MiB";
    case GiB:
      return no_type ? std::to_string(TO_GB(size)) : std::to_string(TO_GB(size)) + "GiB";
  }

  return no_type ? std::to_string(size) : std::to_string(size) + "B";
}

std::string Partition_t::getGUIDAsString() const {
  if (isLogical) throw Error("This is not a normal partition object!");
  return gptPart.GetUniqueGUID().AsString();
}

uint32_t Partition_t::getIndex() const {
  if (isLogical) throw Error("This is not a normal partition object!");
  return index;
}

uint64_t Partition_t::getSize(uint32_t sectorSize) const {
  if (isLogical) {
    Helper::garbageCollector collector;
    int fd = Helper::openAndAddToCloseList(std::filesystem::read_symlink(logicalPartitionPath).string(), collector, O_RDONLY);

    if (fd < 0) {
      LOGE << "Cannot open partition file path: " << std::quoted(logicalPartitionPath.string()) << ": " << strerror(errno)
           << std::endl;
      return 0;
    }

    uint64_t size = 0;
    if (ioctl(fd, static_cast<unsigned int>(BLKGETSIZE64), &size) != 0) {
      LOGN(MAP, ERROR) << "ioctl(BLKGETSIZE64) failed for " << std::quoted(logicalPartitionPath.string()) << ": " << strerror(errno)
                       << std::endl;
      return 0;
    }

    return size;
  }

  return gptPart.GetLengthLBA() * sectorSize;
}

uint64_t Partition_t::getStartByte(uint32_t sectorSize) const {
  if (isLogical) throw Error("This is not a normal partition object!");
  return gptPart.GetFirstLBA() * sectorSize;
}

uint64_t Partition_t::getEndByte(uint32_t sectorSize) const {
  if (isLogical) throw Error("This is not a normal partition object!");
  return (gptPart.GetLastLBA() + 1) * sectorSize;
}

GUIDData Partition_t::getGUID() const {
  if (isLogical) throw Error("This is not a normal partition object!");
  return gptPart.GetUniqueGUID();
}

void Partition_t::set(const BasicData &data) {
  if (isLogical) throw Error("This is not a normal partition object!");
  gptPart = data.gptPart;
  tablePath = data.tablePath;
  index = data.index;
}

void Partition_t::setPartitionPath(const std::filesystem::path &path) {
  if (!isLogical) throw Error("This is not a logical partition object!");
  logicalPartitionPath = path;
}

void Partition_t::setIndex(const uint32_t new_index) {
  if (isLogical) throw Error("This is not a normal partition object!");
  index = new_index;
}

void Partition_t::setDiskPath(const std::filesystem::path &path) {
  if (isLogical) throw Error("This is not a normal partition object!");
  tablePath = path;
}

void Partition_t::setDiskName(const std::string &name) {
  if (isLogical) throw Error("This is not a normal partition object!");
  std::filesystem::path p("/dev/block");
  p /= name; // Add name
  setDiskPath(p);
}

void Partition_t::setGptPart(const GPTPart &otherGptPart) {
  if (isLogical) throw Error("This is not a normal partition object!");
  gptPart = otherGptPart;
}

bool Partition_t::isSuperPartition() const {
  if (isLogical) throw Error("This is not a normal partition object!");
  return getGUID() == GUIDData("89A12DE1-5E41-4CB3-8B4C-B1441EB5DA38");
}

bool Partition_t::isLogicalPartition() const { return isLogical; }

bool Partition_t::empty() { return isLogical ? logicalPartitionPath.empty() : !gptPart.IsUsed() && tablePath.empty(); }

bool Partition_t::operator==(const Partition_t &other) const {
  if (isLogical) return logicalPartitionPath == other.logicalPartitionPath;
  return tablePath == other.tablePath && index == other.index && gptPart.GetUniqueGUID() == other.gptPart.GetUniqueGUID();
}

bool Partition_t::operator==(const GUIDData &other) const {
  if (isLogical) throw Error("This is not a normal partition object!");
  return gptPart.GetUniqueGUID() == other;
}

bool Partition_t::operator!=(const Partition_t &other) const { return !(*this == other); }

bool Partition_t::operator!=(const GUIDData &other) const { return !(*this == other); }

Partition_t::operator bool() { return !empty(); }

bool Partition_t::operator!() { return empty(); }

std::ostream &operator<<(std::ostream &os, Partition_t &other) {
  os << "Name: " << other.getName() << std::endl
     << "Logical: " << std::boolalpha << other.isLogical << std::endl
     << "Path: " << other.getPath() << std::endl;

  if (!other.isLogical)
    os << "Disk path: " << other.getDiskPath() << std::endl
       << "Index: " << other.getIndex() << std::endl
       << "GUID: " << other.gptPart.GetUniqueGUID().AsString() << std::endl;

  return os;
}

} // namespace PartitionMap
