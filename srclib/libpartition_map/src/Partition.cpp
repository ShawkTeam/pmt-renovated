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

#include <filesystem>
#include <libpartition_map/lib.hpp>
#include <ostream>

namespace PartitionMap {

GPTPart Partition_t::getGPTPart() { return gptPart; }

GPTPart *Partition_t::getGPTPartRef() { return &gptPart; }

std::filesystem::path Partition_t::getPath() const {
  std::string suffix = (isdigit(diskPath.string().back())) ? "p" : "";
  std::filesystem::path path(diskPath);
  path /= suffix;
  return path.string() + std::to_string(index + 1);
}

std::filesystem::path Partition_t::getDiskPath() const { return diskPath; }

std::filesystem::path Partition_t::getPathByName() {
  return "/dev/block/by-name/" + gptPart.GetDescription();
}

std::string Partition_t::getName() { return gptPart.GetDescription(); }

std::string Partition_t::getDiskName() const { return diskPath.filename(); }

std::string Partition_t::getFormattedSizeString(SizeUnit size_unit, bool no_type) const {
  uint32_t size = getSize();
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

uint32_t Partition_t::getIndex() const { return index; }

uint64_t Partition_t::getSize(uint32_t sectorSize) const {
  return gptPart.GetLengthLBA() * sectorSize;
}

uint64_t Partition_t::getStartByte(uint32_t sectorSize) const {
  return gptPart.GetFirstLBA() * sectorSize;
}

uint64_t Partition_t::getEndByte(uint32_t sectorSize) const {
  return (gptPart.GetLastLBA() + 1) * sectorSize;
}

void Partition_t::set(const BasicData &data) {
  gptPart = data.gptPart;
  diskPath = data.diskPath;
  index = data.index;
}

void Partition_t::setIndex(const uint32_t new_index) { index = new_index; }

void Partition_t::setDiskPath(const std::string &path) { diskPath = path; }

void Partition_t::setGptPart(const GPTPart &otherGptPart) { gptPart = otherGptPart; }

bool Partition_t::isDynamic() const {
  return Extra::hasMagic(Extra::AndroidMagic::SUPER_IMAGE, 8192, getPath());
}

bool Partition_t::empty() { return !gptPart.IsUsed() && diskPath.empty(); }

bool Partition_t::operator==(const Partition_t &other) const {
  return diskPath == other.diskPath && index == other.index &&
         gptPart.GetUniqueGUID() == other.gptPart.GetUniqueGUID();
}

bool Partition_t::operator==(const GUIDData &other) const {
  return gptPart.GetUniqueGUID() == other;
}

bool Partition_t::operator!=(const Partition_t &other) const { return !(*this == other); }

bool Partition_t::operator!=(const GUIDData &other) const { return !(*this == other); }

Partition_t::operator bool() { return !empty(); }

bool Partition_t::operator!() { return empty(); }

std::ostream &operator<<(std::ostream &os, Partition_t &other) {
  os << "Name: " << other.getName() << std::endl
     << "Disk path: " << other.getDiskPath() << std::endl
     << "Index: " << other.getIndex() << std::endl
     << "GUID: " << other.gptPart.GetUniqueGUID().AsString() << std::endl;

  return os;
}

} // namespace PartitionMap
