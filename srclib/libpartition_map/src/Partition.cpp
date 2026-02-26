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
#include <libhelper/management.hpp>
#include <libhelper/functions.hpp>
#include <libhelper/error.hpp>
#include <libpartition_map/lib.hpp>
#include <libpartition_map/redefine_logging_macros.hpp>

namespace PartitionMap {

bool Partition_t::Extra::isReallyLogical(const std::filesystem::path &path) {
  return path.string().find("/mapper/") != std::string::npos;
}

const char *Partition_t::ErrorCategory::name() const noexcept { return "PartitionError"; }

std::string Partition_t::ErrorCategory::message(int ev) const {
  switch (static_cast<Errors>(ev)) {
    case Errors::Success:
      return "Success";
    case Errors::IsNotLogicalObject:
      return "Is not logical partition object";
    case Errors::IsNotNormalObject:
      return "Is not normal partition object";
    case Errors::CannotOpenLogicalPartition:
      return "Cannot open logical partition path";
    case Errors::CannotFill:
      return "The areas where the image does not fill the partition could not be filled with 0x0";
    case Errors::IoctlFailed:
      return "ioctl(BLKGETSIZE64) failed";
    default:
      return "Unknown Error";
  }
}

const Partition_t::ErrorCategory &Partition_t::getErrorCategory() {
  static ErrorCategory instance;
  return instance;
}

Partition_t &Partition_t::AsLogicalPartition(Partition_t &orig, const std::filesystem::path &path) {
  orig.isLogical = true;
  orig.logicalPartitionPath = path;
  orig.gptPart = GPTPart();
  return orig;
}

GPTPart Partition_t::getGPTPart() const {
  std::error_code ec;
  auto result = getGPTPart(ec);

  if (ec) throw ERR << ec.message();
  return result;
}

GPTPart Partition_t::getGPTPart(std::error_code &ec) const noexcept {
  ec.clear();
  if (isLogical) {
    ec = Errors::IsNotNormalObject;
    return {};
  }
  return gptPart;
}

GPTPart *Partition_t::getGPTPartRef() {
  std::error_code ec;
  auto result = getGPTPartRef(ec);

  if (ec) throw ERR << ec.message();
  return result;
}

GPTPart *Partition_t::getGPTPartRef(std::error_code &ec) noexcept {
  ec.clear();
  if (isLogical) {
    ec = Errors::IsNotNormalObject;
    return nullptr;
  }
  return &gptPart;
}

const GPTPart *Partition_t::getGPTPartRef() const {
  std::error_code ec;
  const auto result = getGPTPartRef(ec);

  if (ec) throw ERR << ec.message();
  return result;
}

const GPTPart *Partition_t::getGPTPartRef(std::error_code &ec) const noexcept {
  ec.clear();
  if (isLogical) {
    ec = Errors::IsNotNormalObject;
    return nullptr;
  }
  return &gptPart;
}

std::filesystem::path Partition_t::path() const {
  std::error_code ec;
  return path(ec);
}

std::filesystem::path Partition_t::path(std::error_code &ec) const noexcept {
  const std::string suffix = isdigit(localTablePath.string().back()) ? "p" : "";
  std::filesystem::path path =
      isLogical ? logicalPartitionPath : std::filesystem::path(localTablePath.string() + suffix + std::to_string(localIndex + 1));
  ec = Errors::Success;
  return path;
}

std::filesystem::path Partition_t::absolutePath() const {
  std::error_code ec;
  const auto result = absolutePath(ec);

  if (ec) throw ERR << ec.message();
  return result;
}

std::filesystem::path Partition_t::absolutePath(std::error_code &ec) const noexcept {
  ec = Errors::Success;
  if (isLogical) return std::filesystem::read_symlink(logicalPartitionPath, ec);
  return path();
}

std::filesystem::path &Partition_t::tablePath() {
  std::error_code ec;
  auto &result = tablePath(ec);

  if (ec) throw ERR << ec.message();
  return result;
}

std::filesystem::path &Partition_t::tablePath(std::error_code &ec) noexcept {
  ec.clear();
  if (isLogical) {
    ec = Errors::IsNotNormalObject;
    return logicalPartitionPath;
  }
  return localTablePath;
}

const std::filesystem::path &Partition_t::tablePath() const {
  std::error_code ec;
  auto &result = tablePath(ec);

  if (ec) throw ERR << ec.message();
  return result;
}

const std::filesystem::path &Partition_t::tablePath(std::error_code &ec) const noexcept {
  ec.clear();
  if (isLogical) {
    ec = Errors::IsNotNormalObject;
    return logicalPartitionPath;
  }
  return localTablePath;
}

std::filesystem::path Partition_t::pathByName() const {
  std::error_code ec;
  auto result = pathByName(ec);

  if (ec) throw ERR << ec.message();
  return result;
}

std::filesystem::path Partition_t::pathByName(std::error_code &ec) const noexcept {
  ec.clear();
  if (isLogical) return logicalPartitionPath;
  std::filesystem::path result = "/dev/block/by-name";
  result /= gptPart.GetDescription();

  if (!std::filesystem::exists("/dev/block/by-name", ec) || std::filesystem::read_symlink(result, ec) != path()) return {};
  return result;
}

std::string Partition_t::name() const {
  if (isLogical) return logicalPartitionPath.filename().string();
  return gptPart.GetDescription();
}

std::string Partition_t::tableName() const {
  std::error_code ec;
  auto result = tableName(ec);

  if (ec) throw ERR << ec.message();
  return result;
}

std::string Partition_t::tableName(std::error_code &ec) const noexcept {
  ec.clear();
  if (isLogical) {
    ec = Errors::IsNotNormalObject;
    return {};
  }
  return localTablePath.filename();
}

std::string Partition_t::formattedSizeString(SizeUnit size_unit, bool no_type) const {
  uint64_t size_ = size();
  switch (size_unit) {
    case BYTE:
      return no_type ? std::to_string(size_) : std::to_string(size_) + "B";
    case KiB:
      return no_type ? std::to_string(TO_KB(size_)) : std::to_string(TO_KB(size_)) + "KiB";
    case MiB:
      return no_type ? std::to_string(TO_MB(size_)) : std::to_string(TO_MB(size_)) + "MiB";
    case GiB:
      return no_type ? std::to_string(TO_GB(size_)) : std::to_string(TO_GB(size_)) + "GiB";
  }

  return no_type ? std::to_string(size_) : std::to_string(size_) + "B";
}

std::string Partition_t::GUIDAsString() const {
  std::error_code ec;
  auto result = GUIDAsString(ec);

  if (ec) throw ERR << ec.message();
  return result;
}

std::string Partition_t::GUIDAsString(std::error_code &ec) const noexcept {
  ec.clear();
  if (isLogical) {
    ec = Errors::IsNotNormalObject;
    return {};
  }
  return gptPart.GetUniqueGUID().AsString();
}

uint32_t &Partition_t::index() {
  std::error_code ec;
  auto &result = index(ec);

  if (ec) throw ERR << ec.message();
  return result;
}

uint32_t &Partition_t::index(std::error_code &ec) noexcept {
  ec.clear();
  if (isLogical) {
    ec = Errors::IsNotNormalObject;
    return localIndex;
  }
  return localIndex;
}

const uint32_t &Partition_t::index() const {
  std::error_code ec;
  auto &result = index(ec);

  if (ec) throw ERR << ec.message();
  return result;
}

const uint32_t &Partition_t::index(std::error_code &ec) const noexcept {
  ec.clear();
  if (isLogical) {
    ec = Errors::IsNotNormalObject;
    return localIndex;
  }
  return localIndex;
}

uint64_t Partition_t::size(uint32_t sectorSize) const {
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
      LOGE << "ioctl(BLKGETSIZE64) failed for " << std::quoted(logicalPartitionPath.string()) << ": " << strerror(errno) << std::endl;
      return 0;
    }

    return size;
  }

  return gptPart.GetLengthLBA() * sectorSize;
}

uint64_t Partition_t::start(std::error_code &ec, uint32_t sectorSize) const noexcept {
  ec.clear();
  if (isLogical) {
    ec = Errors::IsNotNormalObject;
    return std::numeric_limits<uint64_t>::max();
  }
  return gptPart.GetFirstLBA() * sectorSize;
}

uint64_t Partition_t::start(uint32_t sectorSize) const {
  std::error_code ec;
  const auto result = start(ec, sectorSize);

  if (ec) throw ERR << ec.message();
  return result;
}

uint64_t Partition_t::end(std::error_code &ec, uint32_t sectorSize) const noexcept {
  ec.clear();
  if (isLogical) {
    ec = Errors::IsNotNormalObject;
    return std::numeric_limits<uint64_t>::max();
  }
  return (gptPart.GetLastLBA() + 1) * sectorSize;
}

uint64_t Partition_t::end(uint32_t sectorSize) const {
  std::error_code ec;
  const auto result = end(ec, sectorSize);

  if (ec) throw ERR << ec.message();
  return result;
}

GUIDData Partition_t::GUID(std::error_code &ec) const noexcept {
  if (isLogical) {
    ec = Errors::IsNotNormalObject;
    return {};
  }
  return gptPart.GetUniqueGUID();
}

GUIDData Partition_t::GUID() const {
  std::error_code ec;
  const auto result = GUID(ec);

  if (ec) throw ERR << ec.message();
  return result;
}

bool Partition_t::dump(std::error_code &ec, const std::filesystem::path &destination, uint64_t bufsize) const noexcept {
  ec.clear();
  Helper::garbageCollector collector;
  const std::filesystem::path dest = destination.empty() ? (std::filesystem::path("./") += name() + ".img") : destination;
  const std::filesystem::path toOpen = isLogical ? absolutePath() : path();

  const int partitionfd = Helper::openAndAddToCloseList(toOpen, collector, O_RDONLY);
  if (partitionfd < 0) {
    ec = std::make_error_code(std::errc::io_error);
    return false;
  }

  const int outfd = Helper::openAndAddToCloseList(dest, collector, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (outfd < 1) {
    if (errno == EPERM || errno == EACCES)
      ec = std::make_error_code(std::errc::permission_denied);
    else
      ec = std::make_error_code(std::errc::io_error);
    return false;
  }

  const uint64_t bufferSize = std::min<uint64_t>(bufsize, size());
  std::vector<char> buffer(bufferSize);

  const uint64_t totalBytesToRead = size();
  uint64_t bytesReadSoFar = 0;

  while (bytesReadSoFar < totalBytesToRead) {
    uint64_t toRead = std::min(bufferSize, totalBytesToRead - bytesReadSoFar);

    ssize_t bytesRead = read(partitionfd, buffer.data(), toRead);
    if (bytesRead <= 0) {
      if (errno == EPERM)
        ec = std::make_error_code(std::errc::permission_denied);
      else
        ec = std::make_error_code(std::errc::io_error);
      return false;
    }

    if (const ssize_t bytesWritten = ::write(outfd, buffer.data(), bytesRead); bytesWritten != bytesRead) {
      if (errno == EPERM)
        ec = std::make_error_code(std::errc::permission_denied);
      else
        ec = std::make_error_code(std::errc::io_error);
      return false;
    }

    bytesReadSoFar += bytesRead;
  }

  return bytesReadSoFar == totalBytesToRead;
}

bool Partition_t::dump(const std::filesystem::path &destination, uint64_t bufsize) const {
  std::error_code ec;
  const auto result = dump(ec, destination, bufsize);

  if (ec) throw ERR << ec.message();
  return result;
}

bool Partition_t::write(std::error_code &ec, const std::filesystem::path &image, uint64_t bufsize) noexcept {
  ec.clear();
  Helper::garbageCollector collector;

  const int64_t imageSize = Helper::fileSize(image);
  if (imageSize < 0) {
    ec = std::make_error_code(std::errc::io_error);
    return false;
  }
  if (imageSize > size()) {
    ec = std::make_error_code(std::errc::file_too_large);
    return false;
  }

  const std::filesystem::path toWrite = isLogical ? absolutePath() : path();
  const int partitionfd = Helper::openAndAddToCloseList(toWrite, collector, O_WRONLY);
  if (partitionfd < 0) {
    if (errno == EPERM || errno == EACCES)
      ec = std::make_error_code(std::errc::permission_denied);
    else
      ec = std::make_error_code(std::errc::io_error);
    return false;
  }

  const int imagefd = Helper::openAndAddToCloseList(image, collector, O_RDONLY);
  if (imagefd < 0) {
    if (errno == EPERM || errno == EACCES)
      ec = std::make_error_code(std::errc::permission_denied);
    else
      ec = std::make_error_code(std::errc::io_error);
    return false;
  }

  uint64_t bytesWrittenSoFar = 0;
  const uint64_t bufferSize = std::min<uint64_t>(bufsize, size());
  std::vector<char> buffer(bufferSize);

  while (bytesWrittenSoFar < imageSize) {
    uint64_t toRead = std::min(bufferSize, imageSize - bytesWrittenSoFar);

    ssize_t bytesRead = read(imagefd, buffer.data(), toRead);
    if (bytesRead <= 0) {
      if (errno == EPERM)
        ec = std::make_error_code(std::errc::permission_denied);
      else
        ec = std::make_error_code(std::errc::io_error);
      return false;
    }

    if (const ssize_t bytesWritten = ::write(partitionfd, buffer.data(), bytesRead); bytesWritten != bytesRead) {
      if (errno == EPERM)
        ec = std::make_error_code(std::errc::permission_denied);
      else
        ec = std::make_error_code(std::errc::io_error);
      return false;
    }

    bytesWrittenSoFar += bytesRead;
  }

  if (bytesWrittenSoFar < size()) {
    uint64_t remainingBytes = size() - bytesWrittenSoFar;
    std::ranges::fill(buffer, 0x00);

    while (remainingBytes > 0) {
      uint64_t toWriteSize = std::min<uint64_t>(buffer.size(), remainingBytes);
      ssize_t written = ::write(partitionfd, buffer.data(), toWriteSize);

      if (written <= 0) {
        LOGE << "Cannot fill the outside of partition (of image file): " << strerror(errno);
        if (errno == EPERM)
          ec = std::make_error_code(std::errc::permission_denied);
        else
          ec = std::make_error_code(std::errc::io_error);
        return false;
      }
      remainingBytes -= written;
    }
  }

  fsync(partitionfd);
  return bytesWrittenSoFar == imageSize;
}

bool Partition_t::write(const std::filesystem::path &image, uint64_t bufsize) {
  std::error_code ec;
  const auto result = write(ec, image, bufsize);

  if (ec) throw ERR << ec.message();
  return result;
}

void Partition_t::set(const BasicData &data) {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  gptPart = data.gptPart;
  localTablePath = data.tablePath;
  localIndex = data.index;
}

void Partition_t::setPartitionPath(const std::filesystem::path &path) {
  if (!isLogical) throw ERR << "This is not a logical partition object!";
  logicalPartitionPath = path;
}

void Partition_t::setIndex(const uint32_t new_index) {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  localIndex = new_index;
}

void Partition_t::setDiskPath(const std::filesystem::path &path) {
  if (isLogical) throw ERR << "This is not a normal partition object!";
  localTablePath = path;
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
  return GUID() == GUIDData("89A12DE1-5E41-4CB3-8B4C-B1441EB5DA38");
}

bool Partition_t::isLogicalPartition() const { return isLogical; }

bool Partition_t::empty() const { return isLogical ? logicalPartitionPath.empty() : !gptPart.IsUsed() && localTablePath.empty(); }

bool Partition_t::operator==(const Partition_t &other) const {
  if (isLogical) return logicalPartitionPath == other.logicalPartitionPath;
  return localTablePath == other.localTablePath && localIndex == other.localIndex &&
         gptPart.GetUniqueGUID() == other.gptPart.GetUniqueGUID();
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
    localTablePath = std::move(other.localTablePath);
    localIndex = other.localIndex;
    logicalPartitionPath = std::move(other.logicalPartitionPath);
    gptPart = other.gptPart;
    isLogical = other.isLogical;

    other.localIndex = 0;
    other.gptPart = GPTPart();
    other.isLogical = false;
  }

  return *this;
}

std::ostream &operator<<(std::ostream &os, Partition_t &other) {
  os << "Name: " << other.name() << std::endl
     << "Logical: " << std::boolalpha << other.isLogical << std::endl
     << "Path: " << other.path() << std::endl;

  if (!other.isLogical)
    os << "Disk path: " << other.tablePath() << std::endl
       << "Index: " << other.index() << std::endl
       << "GUID: " << other.gptPart.GetUniqueGUID().AsString() << std::endl;

  return os;
}

std::error_code make_error_code(Partition_t::Errors ec) { return {static_cast<int>(ec), Partition_t::getErrorCategory()}; }

} // namespace PartitionMap
