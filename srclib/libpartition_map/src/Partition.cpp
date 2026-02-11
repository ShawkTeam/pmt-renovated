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

#include <filesystem>
#include <ostream>
#include <unistd.h>
#include <asm-generic/fcntl.h>
#include <libpartition_map/lib.hpp>
#include <libpartition_map/redefine_logging_macros.hpp>

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

GPTPart Partition_t::getGPTPart() const {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  return gptPart;
}

GPTPart *Partition_t::getGPTPartRef() {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  return &gptPart;
}

const GPTPart *Partition_t::getGPTPartRef() const {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  return &gptPart;
}

std::filesystem::path Partition_t::getPath() const {
  const std::string suffix = isdigit(tablePath.string().back()) ? "p" : "";
  std::filesystem::path path =
      isLogical ? logicalPartitionPath : std::filesystem::path(tablePath.string() + suffix + std::to_string(index + 1));
  return path;
}

std::filesystem::path Partition_t::getAbsolutePath() const {
  if (isLogical) return std::filesystem::read_symlink(logicalPartitionPath);
  return getPath();
}

std::filesystem::path &Partition_t::getTablePath() {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  return tablePath;
}

const std::filesystem::path &Partition_t::getTablePath() const {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  return tablePath;
}

std::filesystem::path Partition_t::getPathByName() const {
  if (isLogical) return logicalPartitionPath;
  std::filesystem::path result = "/dev/block/by-name";
  result /= gptPart.GetDescription();

  if (!std::filesystem::exists("/dev/block/by-name") || std::filesystem::read_symlink(result) != getPath()) return {};
  return result;
}

std::string Partition_t::getName() const {
  if (isLogical) return logicalPartitionPath.filename().string();
  return gptPart.GetDescription();
}

std::string Partition_t::getTableName() const {
  if (isLogical) throw ERR << "This is not a normal partition object!";
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
  if (isLogical) throw ERR << "This is not a normal partition object!";
  return gptPart.GetUniqueGUID().AsString();
}

uint32_t &Partition_t::getIndex() {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  return index;
}

const uint32_t &Partition_t::getIndex() const {
  if (isLogical) throw ERR << "This is not a normal partition object!";
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
  if (isLogical) throw ERR << "This is not a normal partition object!";
  return gptPart.GetFirstLBA() * sectorSize;
}

uint64_t Partition_t::getEndByte(uint32_t sectorSize) const {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  return (gptPart.GetLastLBA() + 1) * sectorSize;
}

GUIDData Partition_t::getGUID() const {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  return gptPart.GetUniqueGUID();
}

bool Partition_t::dumpImage(const std::filesystem::path &destination, uint64_t bufsize) const {
  Helper::garbageCollector collector;
  const std::filesystem::path dest = destination.empty() ? (std::filesystem::path("./") += getName() + ".img") : destination;
  const std::filesystem::path toOpen = isLogical ? getAbsolutePath() : getPath();

  const int partitionfd = Helper::openAndAddToCloseList(toOpen, collector, O_RDONLY);
  if (partitionfd < 0) throw ERR << "Cannot open " << std::quoted(toOpen.string()) << ": " << strerror(errno);

  const int outfd = Helper::openAndAddToCloseList(dest, collector, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (outfd < 1) throw ERR << "Cannot create/open " << std::quoted(dest.string()) << ": " << strerror(errno);

  const uint64_t bufferSize = std::min<uint64_t>(bufsize, getSize());
  std::vector<char> buffer(bufferSize);

  const uint64_t totalBytesToRead = getSize();
  uint64_t bytesReadSoFar = 0;

  while (bytesReadSoFar < totalBytesToRead) {
    uint64_t toRead = std::min(bufferSize, totalBytesToRead - bytesReadSoFar);

    ssize_t bytesRead = read(partitionfd, buffer.data(), toRead);
    if (bytesRead <= 0) throw ERR << "Cannot read " << std::quoted(toOpen.string()) << ": " << strerror(errno);

    if (const ssize_t bytesWritten = write(outfd, buffer.data(), bytesRead); bytesWritten != bytesRead)
      throw ERR << "Write error at " << bytesReadSoFar << " bytes: " << strerror(errno);

    bytesReadSoFar += bytesRead;
  }

  return bytesReadSoFar == totalBytesToRead;
}

bool Partition_t::writeImage(const std::filesystem::path &image, uint64_t bufsize) {
  Helper::garbageCollector collector;

  const int64_t imageSize = Helper::fileSize(image);
  if (imageSize < 0) throw ERR << "Cannot get size of: " << std::quoted(image.string()) << ": " << strerror(errno);
  if (imageSize > getSize())
    throw ERR << std::quoted(image.string()) << " is larger than " << std::quoted(getName()) << " (" << imageSize << " > " << getSize()
              << ").";

  const std::filesystem::path toWrite = isLogical ? getAbsolutePath() : getPath();
  const int partitionfd = Helper::openAndAddToCloseList(toWrite, collector, O_WRONLY);
  if (partitionfd < 0) throw ERR << "Cannot open " << toWrite.string() << ": " << strerror(errno);

  const int imagefd = Helper::openAndAddToCloseList(image, collector, O_RDONLY);
  if (imagefd < 0) throw ERR << "Cannot open " << image.string() << ": " << strerror(errno);

  uint64_t bytesWrittenSoFar = 0;
  const uint64_t bufferSize = std::min<uint64_t>(bufsize, getSize());
  std::vector<char> buffer(bufferSize);

  while (bytesWrittenSoFar < imageSize) {
    uint64_t toRead = std::min(bufferSize, imageSize - bytesWrittenSoFar);

    ssize_t bytesRead = read(imagefd, buffer.data(), toRead);
    if (bytesRead <= 0) throw ERR << "Cannot read " << std::quoted(image.string()) << ": " << strerror(errno);

    if (const ssize_t bytesWritten = write(partitionfd, buffer.data(), bytesRead); bytesWritten != bytesRead)
      throw ERR << "Write error at " << bytesWrittenSoFar << " bytes. The partition may be damaged! " << strerror(errno);

    bytesWrittenSoFar += bytesRead;
  }

  if (bytesWrittenSoFar < getSize()) {
    uint64_t remainingBytes = getSize() - bytesWrittenSoFar;
    std::ranges::fill(buffer, 0x00);

    while (remainingBytes > 0) {
#ifdef __LP64__
      uint64_t toWriteSize = std::min(buffer.size(), remainingBytes);
#else
      uint64_t toWriteSize = std::min<uint64_t>(buffer.size(), remainingBytes);
#endif
      ssize_t written = write(partitionfd, buffer.data(), toWriteSize);

      if (written <= 0) throw ERR << "Cannot fill the outside of partition (of image file): " << strerror(errno);
      remainingBytes -= written;
    }
  }

  fsync(partitionfd);
  return bytesWrittenSoFar == imageSize;
}

void Partition_t::set(const BasicData &data) {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  gptPart = data.gptPart;
  tablePath = data.tablePath;
  index = data.index;
}

void Partition_t::setPartitionPath(const std::filesystem::path &path) {
  if (!isLogical) throw ERR << "This is not a logical partition object!";
  logicalPartitionPath = path;
}

void Partition_t::setIndex(const uint32_t new_index) {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  index = new_index;
}

void Partition_t::setDiskPath(const std::filesystem::path &path) {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  tablePath = path;
}

void Partition_t::setDiskName(const std::string &name) {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  std::filesystem::path p("/dev/block");
  p /= name; // Add name
  setDiskPath(p);
}

void Partition_t::setGptPart(const GPTPart &otherGptPart) {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  gptPart = otherGptPart;
}

bool Partition_t::isSuperPartition() const {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  return getGUID() == GUIDData("89A12DE1-5E41-4CB3-8B4C-B1441EB5DA38");
}

bool Partition_t::isLogicalPartition() const { return isLogical; }

bool Partition_t::empty() const { return isLogical ? logicalPartitionPath.empty() : !gptPart.IsUsed() && tablePath.empty(); }

bool Partition_t::operator==(const Partition_t &other) const {
  if (isLogical) return logicalPartitionPath == other.logicalPartitionPath;
  return tablePath == other.tablePath && index == other.index && gptPart.GetUniqueGUID() == other.gptPart.GetUniqueGUID();
}

bool Partition_t::operator==(const GUIDData &other) const {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  return gptPart.GetUniqueGUID() == other;
}

bool Partition_t::operator!=(const Partition_t &other) const { return !(*this == other); }

bool Partition_t::operator!=(const GUIDData &other) const { return !(*this == other); }

Partition_t::operator bool() const { return !empty(); }

bool Partition_t::operator!() const { return empty(); }

GPTPart *Partition_t::operator*() { return getGPTPartRef(); }

const GPTPart *Partition_t::operator*() const { return getGPTPartRef(); }

Partition_t &Partition_t::operator=(Partition_t &&other) noexcept {
  if (this != &other) {
    tablePath = std::move(other.tablePath);
    index = other.index;
    logicalPartitionPath = std::move(other.logicalPartitionPath);
    gptPart = other.gptPart;
    isLogical = other.isLogical;

    other.index = 0;
    other.gptPart = GPTPart();
    other.isLogical = false;
  }

  return *this;
}

std::ostream &operator<<(std::ostream &os, Partition_t &other) {
  os << "Name: " << other.getName() << std::endl
     << "Logical: " << std::boolalpha << other.isLogical << std::endl
     << "Path: " << other.getPath() << std::endl;

  if (!other.isLogical)
    os << "Disk path: " << other.getTablePath() << std::endl
       << "Index: " << other.getIndex() << std::endl
       << "GUID: " << other.gptPart.GetUniqueGUID().AsString() << std::endl;

  return os;
}

} // namespace PartitionMap
