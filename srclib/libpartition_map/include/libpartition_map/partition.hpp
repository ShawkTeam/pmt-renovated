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

#ifndef LIBPARTITION_MAP_PARTITION_HPP
#define LIBPARTITION_MAP_PARTITION_HPP

#if __cplusplus < 202002L
#error "libpartition_map/partition.hpp is requires C++20 or higher C++ standarts."
#endif

#include <filesystem>
#include <ostream>
#include <tuple>
#include <asm-generic/fcntl.h>
#include <gpt.h>
#include <libhelper/management.hpp>
#include <libhelper/functions.hpp>
#include <libhelper/error.hpp>
#include <libhelper/logging.hpp>
#include <libhelper/definations.hpp>
#include <libpartition_map/definations.hpp>
#include <libpartition_map/redefine_logging_macros.hpp>

namespace PartitionMap {
enum class Errors {
  Success = 0,
  IsNotLogicalObject = 1,
  IsNotNormalObject = 2,
  CannotOpenLogicalPartition = 3,
  CannotFill = 4,
  IoctlFailed = 5
};

class basic_partition_t_errors final : public std::error_category {
public:
  const char *name() const noexcept override { return "PartitionError"; }

  std::string message(int ev) const override {
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
};

} // namespace PartitionMap

template <> struct std::is_error_code_enum<PartitionMap::Errors> : std::true_type {};

namespace PartitionMap {
template <typename slot_type, typename size_type, typename path_type>
  requires IsSlotType<slot_type> && IsSizeType<size_type> && IsPathTypeLike<path_type>
class BasicPartition_t {
  path_type localTablePath;       // The table path to which the partition belongs (like /dev/block/sdc).
  path_type logicalPartitionPath; // Path of logical partition.
  slot_type localIndex = 0;       // The actual index of the partition within the table.
  mutable GPTPart gptPart;        // Complete data for the partition.

  bool isLogical = false; // This class contains a logical partition?

  template <typename Tuple, std::size_t... I> void assign_from_tuple_impl(Tuple &&t, std::index_sequence<I...>) {
    (process_ctor(std::get<I>(t)), ...);
  }

  template <typename Tuple> void assign_from_tuple(Tuple &&t) {
    assign_from_tuple_impl(std::forward<Tuple>(t), std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
  }

  void process_ctor(const slot_type &index) { localIndex = index; }
  void process_ctor(const GPTPart &part) { gptPart = part; }
  void process_ctor(const path_type &path) { localTablePath = path; }

public:
  class Extra {
  public:
    static bool isReallyLogical(const path_type &path) { return path.string().find("/mapper/") != std::string::npos; }
  };

  static const basic_partition_t_errors &getErrorCategory() {
    static basic_partition_t_errors instance;
    return instance;
  }

  using BasicData = basic_data_base<slot_type>;
  using ErrorCategory = basic_partition_t_errors;
  using IOCallback = std::function<void(size_type, size_type)>; // First = written/readed size, second = total size.

  static BasicPartition_t &AsLogicalPartition(BasicPartition_t &orig, const path_type &path) {
    orig.isLogical = true;
    orig.logicalPartitionPath = path;
    orig.gptPart = GPTPart();
    return orig;
  }

  BasicPartition_t() : gptPart(GPTPart()) {}                 // BasicPartition_t<...> partititon
  BasicPartition_t(const BasicPartition_t &other) = default; // BasicPartition_t<...> partition(otherPartition)
  BasicPartition_t(BasicPartition_t &&other) noexcept        // BasicPartition_t<...> partition(std::move(otherPartition))
      : localTablePath(std::move(other.localTablePath)), logicalPartitionPath(std::move(other.logicalPartitionPath)),
        localIndex(other.localIndex), gptPart(other.gptPart), isLogical(other.isLogical) {
    other.localIndex = 0;
    other.gptPart = GPTPart();
    other.isLogical = false;
  }

  template <typename... Args>
    requires(sizeof...(Args) == 3) &&
            ((is_gptpart_decay<Args> || is_slot_type_decay<Args, slot_type> || is_string_or_path_decay<Args, path_type>) && ...)
  explicit BasicPartition_t(Args &&...args) { // BasicPartition_t<...> partition(myGptPart, "/dev/block/sda", 4); For normal
                                              // partitions. The order of arguments may differ from the example.
    auto tuple = std::make_tuple(std::forward<Args>(args)...);
    assign_from_tuple(tuple);
  }

  explicit BasicPartition_t(const basic_data_base<slot_type>
                                &input) // BasicPartition_t<...> partition({myGptPart, 4, "/dev/block/sda"}); For normal partitions.
      : localTablePath(input.tablePath), localIndex(input.index), gptPart(input.gptPart) {}

  explicit BasicPartition_t(const path_type &path) /* NOLINT(modernize-pass-by-value) */
      : logicalPartitionPath(path), gptPart(GPTPart()), isLogical(true) {
  } // BasicPartition_t<...> logicalPartition("/dev/block/mapper/system"); For logical partitions.

  // Get copy of GPTPart data.
  GPTPart getGPTPart() const {
    std::error_code ec;
    auto result = getGPTPart(ec);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get copy of GPTPart data.
  GPTPart getGPTPart(std::error_code &ec) const noexcept {
    ec.clear();
    if (isLogical) {
      ec = Errors::IsNotNormalObject;
      return {};
    }
    return gptPart;
  }

  // Get reference of GPTPart data (non-const reference).
  GPTPart *getGPTPartRef() {
    std::error_code ec;
    auto result = getGPTPartRef(ec);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get reference of GPTPart data (non-const reference).
  GPTPart *getGPTPartRef(std::error_code &ec) noexcept {
    ec.clear();
    if (isLogical) {
      ec = Errors::IsNotNormalObject;
      return nullptr;
    }
    return &gptPart;
  }

  // Get reference of GPTPart data (const reference).
  const GPTPart *getGPTPartRef() const {
    std::error_code ec;
    const auto result = getGPTPartRef(ec);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get reference of GPTPart data (const reference).
  const GPTPart *getGPTPartRef(std::error_code &ec) const noexcept {
    ec.clear();
    if (isLogical) {
      ec = Errors::IsNotNormalObject;
      return nullptr;
    }
    return &gptPart;
  }

  // Get partition path (like /dev/block/sdc4).
  path_type path() const {
    std::error_code ec;
    return path(ec);
  }

  // Get partition path (like /dev/block/sdc4).
  path_type path(std::error_code &ec) const noexcept {
    const std::string suffix = isdigit(localTablePath.string().back()) ? "p" : "";
    path_type path = isLogical ? logicalPartitionPath : path_type(localTablePath.string() + suffix + std::to_string(localIndex + 1));
    ec = Errors::Success;
    return path;
  }

  // Get absolute partition path (returns std::filesystem::read_symlink(getPath()) for normal partitions).
  path_type absolutePath() const {
    std::error_code ec;
    const auto result = absolutePath(ec);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get absolute partition path (returns std::filesystem::read_symlink(getPath()) for normal partitions).
  path_type absolutePath(std::error_code &ec) const noexcept {
    ec = Errors::Success;
    if (isLogical) return std::filesystem::read_symlink(logicalPartitionPath, ec);
    return path();
  }

  // Get tablePath variable (const reference).
  const path_type &tablePath() const {
    std::error_code ec;
    auto &result = tablePath(ec);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get tablePath variable (const reference).
  const path_type &tablePath(std::error_code &ec) const noexcept {
    ec.clear();
    if (isLogical) {
      ec = Errors::IsNotNormalObject;
      return logicalPartitionPath;
    }
    return localTablePath;
  }

  // Get tablePath variable (non-const reference).
  path_type &tablePath() {
    std::error_code ec;
    auto &result = tablePath(ec);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get tablePath variable (non-const reference).
  path_type &tablePath(std::error_code &ec) noexcept {
    ec.clear();
    if (isLogical) {
      ec = Errors::IsNotNormalObject;
      return logicalPartitionPath;
    }
    return localTablePath;
  }

  // Get partition path by name.
  path_type pathByName() const {
    std::error_code ec;
    auto result = pathByName(ec);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get partition path by name.
  path_type pathByName(std::error_code &ec) const noexcept {
    ec.clear();
    if (isLogical) return logicalPartitionPath;
    path_type result = "/dev/block/by-name";
    result.append(gptPart.GetDescription());

    if (!std::filesystem::exists("/dev/block/by-name", ec) || std::filesystem::read_symlink(result, ec) != path()) return {};
    return result;
  }

  // Get partition name.
  std::string name() const {
    if (isLogical) return logicalPartitionPath.filename().string();
    return gptPart.GetDescription();
  }

  // Get table name.
  std::string tableName() const {
    std::error_code ec;
    auto result = tableName(ec);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get table name.
  std::string tableName(std::error_code &ec) const noexcept {
    ec.clear();
    if (isLogical) {
      ec = Errors::IsNotNormalObject;
      return {};
    }
    return localTablePath.filename();
  }

  // Get partition size as formatted string.
  std::string formattedSizeString(const SizeUnit size_unit, bool no_type = false) const {
    size_type size_ = size();
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

  // Get partition GUID as string.
  std::string GUIDAsString() const {
    std::error_code ec;
    auto result = GUIDAsString(ec);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get partition GUID as string.
  std::string GUIDAsString(std::error_code &ec) const noexcept {
    ec.clear();
    if (isLogical) {
      ec = Errors::IsNotNormalObject;
      return {};
    }
    return gptPart.GetUniqueGUID().AsString();
  }

  // Get partition index in GPT table (const) reference.
  const slot_type &index() const {
    std::error_code ec;
    auto &result = index(ec);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get partition index in GPT table (const) reference.
  const slot_type &index(std::error_code &ec) const noexcept {
    ec.clear();
    if (isLogical) {
      ec = Errors::IsNotNormalObject;
      return localIndex;
    }
    return localIndex;
  }

  // Get partition index in GPT table (non-const) as reference.
  slot_type &index() {
    std::error_code ec;
    auto &result = index(ec);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get partition index in GPT table (non-const) as reference.
  slot_type &index(std::error_code &ec) noexcept {
    ec.clear();
    if (isLogical) {
      ec = Errors::IsNotNormalObject;
      return localIndex;
    }
    return localIndex;
  }

  // Get partition size in bytes.
  size_type size(size_type sectorSize = 4096) const {
    if (isLogical) {
      auto fd = Helper::UniqueFD(std::filesystem::read_symlink(logicalPartitionPath).string(), O_RDONLY);

      if (!fd)
        throw Error("Cannot open partition file path: {}: {}", std::quoted_string(logicalPartitionPath.c_str()), std::strerror(errno));

      size_type size = 0;
      if (fd.ioctl(static_cast<unsigned int>(BLKGETSIZE64), &size) != 0)
        throw Error("ioctl(BLKGETSIZE64) failed: {}: {}", std::quoted_string(logicalPartitionPath.c_str()), std::strerror(errno));

      return size;
    }

    return gptPart.GetLengthLBA() * sectorSize;
  }

  // Get starting byte address.
  size_type start(std::error_code &ec, size_type sectorSize = 4096) const noexcept {
    ec.clear();
    if (isLogical) {
      ec = Errors::IsNotNormalObject;
      return std::numeric_limits<size_type>::max();
    }
    return gptPart.GetFirstLBA() * sectorSize;
  }

  // Get starting byte address.
  size_type start(size_type sectorSize = 4096) const {
    std::error_code ec;
    const auto result = start(ec, sectorSize);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get ending byte address.
  size_type end(std::error_code &ec, size_type sectorSize = 4096) const noexcept {
    ec.clear();
    if (isLogical) {
      ec = Errors::IsNotNormalObject;
      return std::numeric_limits<size_type>::max();
    }
    return (gptPart.GetLastLBA() + 1) * sectorSize;
  }

  // Get ending byte address.
  size_type end(size_type sectorSize = 4096) const {
    std::error_code ec;
    const auto result = end(ec, sectorSize);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get partition GUID.
  GUIDData GUID() const {
    std::error_code ec;
    const auto result = GUID(ec);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Get partition GUID.
  GUIDData GUID(std::error_code &ec) const noexcept {
    if (isLogical) {
      ec = Errors::IsNotNormalObject;
      return {};
    }
    return gptPart.GetUniqueGUID();
  }

  // Dump image of partition.
  [[maybe_unused]] bool dump(const path_type &destination = "", size_type bufsize = MB(1), IOCallback callback = nullptr) const {

    std::error_code ec;
    const auto result = dump(ec, destination, bufsize, callback);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Dump image of partition.
  [[maybe_unused]] bool dump(std::error_code &ec, const path_type &destination = "", size_type bufsize = MB(1),
                             IOCallback callback = nullptr) const noexcept {

    ec.clear();
    const path_type dest = destination.empty() ? (path_type("./") += name() + ".img") : destination;
    const path_type toOpen = isLogical ? absolutePath() : path();

    const auto partitionfd = Helper::UniqueFD(toOpen, O_RDONLY);
    if (!partitionfd) {
      ec = std::make_error_code(std::errc::io_error);
      return false;
    }

    auto outfd = Helper::UniqueFD(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (!outfd) {
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

      ssize_t bytesRead = partitionfd.read(buffer.data(), toRead);
      if (bytesRead <= 0) {
        if (errno == EPERM)
          ec = std::make_error_code(std::errc::permission_denied);
        else
          ec = std::make_error_code(std::errc::io_error);
        return false;
      }

      if (const ssize_t bytesWritten = outfd.write(buffer.data(), bytesRead); bytesWritten != bytesRead) {
        if (errno == EPERM)
          ec = std::make_error_code(std::errc::permission_denied);
        else
          ec = std::make_error_code(std::errc::io_error);
        return false;
      }

      bytesReadSoFar += bytesRead;
      if (callback) callback(bytesReadSoFar, totalBytesToRead);
    }

    return bytesReadSoFar == totalBytesToRead;
  }

  // Write input image to partition.
  [[maybe_unused]] bool write(std::error_code &ec, const path_type &image, size_type bufsize = MB(1),
                              IOCallback callback = nullptr) noexcept {

    ec.clear();

    const int64_t imageSize = Helper::fileSize(image);
    if (imageSize < 0) {
      ec = std::make_error_code(std::errc::io_error);
      return false;
    }
    if (imageSize > size()) {
      ec = std::make_error_code(std::errc::file_too_large);
      return false;
    }

    const path_type toWrite = isLogical ? absolutePath() : path();
    auto partitionfd = Helper::UniqueFD(toWrite, O_WRONLY);
    if (!partitionfd) {
      if (errno == EPERM || errno == EACCES)
        ec = std::make_error_code(std::errc::permission_denied);
      else
        ec = std::make_error_code(std::errc::io_error);
      return false;
    }

    auto imagefd = Helper::UniqueFD(image, O_RDONLY);
    if (!imagefd) {
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

      ssize_t bytesRead = imagefd.read(buffer.data(), toRead);
      if (bytesRead <= 0) {
        if (errno == EPERM)
          ec = std::make_error_code(std::errc::permission_denied);
        else
          ec = std::make_error_code(std::errc::io_error);
        return false;
      }

      if (const ssize_t bytesWritten = partitionfd.write(buffer.data(), bytesRead); bytesWritten != bytesRead) {
        if (errno == EPERM)
          ec = std::make_error_code(std::errc::permission_denied);
        else
          ec = std::make_error_code(std::errc::io_error);
        return false;
      }

      bytesWrittenSoFar += bytesRead;
      if (callback) callback(bytesWrittenSoFar, imageSize);
    }

    if (bytesWrittenSoFar < size()) {
      uint64_t remainingBytes = size() - bytesWrittenSoFar;
      std::ranges::fill(buffer, 0x00);

      while (remainingBytes > 0) {
        uint64_t toWriteSize = std::min<uint64_t>(buffer.size(), remainingBytes);
        ssize_t written = partitionfd.write(buffer.data(), toWriteSize);

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

    partitionfd.fsync();
    return bytesWrittenSoFar == imageSize;
  }

  // Write input image to partition.
  [[maybe_unused]] bool write(const path_type &image, size_type bufsize = MB(1), IOCallback callback = nullptr) {
    std::error_code ec;
    const auto result = write(ec, image, bufsize, callback);

    if (ec) throw Error("{}", ec.message());
    return result;
  }

  // Set GPTPart object, index and table path.
  void set(const basic_data_base<slot_type> &data) {
    if (isLogical) throw Error("This is not a normal partition object!");
    gptPart = data.gptPart;
    localTablePath = data.tablePath;
    localIndex = data.index;
  }

  // Set partition path. Only for logical partitions.
  void setPartitionPath(const path_type &path) {
    if (!isLogical) throw Error("This is not a logical partition object!");
    logicalPartitionPath = path;
  }

  // Set partition index.
  void setIndex(slot_type new_index) {
    if (isLogical) throw Error("This is not a normal partition object!");
    localIndex = new_index;
  }

  // Set table path.
  void setDiskPath(const path_type &path) {
    if (isLogical) throw Error("This is not a normal partition object!");
    localTablePath = path;
  }

  // Set table name (automatically adds /dev/block and uses setDiskPath()).
  void setDiskName(const std::string &name) {
    if (isLogical) throw Error("This is not a normal partition object!");
    path_type p("/dev/block");
    p.append(name); // Add name
    setDiskPath(p);
  }

  // Set GPTPart object.
  void setGptPart(const GPTPart &otherGptPart) {
    if (isLogical) throw Error("This is not a normal partition object!");
    gptPart = otherGptPart;
  }

  // Checks whether the partition is dynamic or not.
  bool isSuperPartition() const {
    if (isLogical) throw Error("This is not a normal partition object!");
    return GUID() == GUIDData("89A12DE1-5E41-4CB3-8B4C-B1441EB5DA38");
  }

  // Checks whether the partition is logical or not.
  bool isLogicalPartition() const { return isLogical; }

  // Checks whether the partition info is empty or not.
  bool empty() const { return isLogical ? logicalPartitionPath.empty() : !gptPart.IsUsed() && localTablePath.empty(); }

  // p1 == p2
  bool operator==(const BasicPartition_t &other) const {
    if (isLogical) return logicalPartitionPath == other.logicalPartitionPath;
    return localTablePath == other.localTablePath && localIndex == other.localIndex &&
           gptPart.GetUniqueGUID() == other.gptPart.GetUniqueGUID();
  }

  // p1 == guid
  bool operator==(const GUIDData &other) const {
    if (isLogical) throw Error("This is not a normal partition object!");
    return gptPart.GetUniqueGUID() == other;
  }

  // p1 != p2
  bool operator!=(const BasicPartition_t &other) const { return !(*this == other); }

  // p1 != guid
  bool operator!=(const GUIDData &other) const { return !(*this == other); }

  // p (returns !empty())
  explicit operator bool() const { return !empty(); }

  // !p (returns empty())
  bool operator!() const { return empty(); }

  // const GPTPart part = *p1
  const GPTPart *operator*() const { return getGPTPart(); }

  // GPTPart part = *pd1
  GPTPart *operator*() { return getGPTPart(); }

  // p1 = p2
  BasicPartition_t &operator=(const BasicPartition_t &other) = default;

  // p1 = std::move(p2)
  BasicPartition_t &operator=(BasicPartition_t &&other) noexcept {
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

  // std::cout << p1
  friend std::ostream &operator<<(std::ostream &os, BasicPartition_t &other) {
    os << "Name: " << other.name() << std::endl
       << "Logical: " << std::boolalpha << other.isLogical << std::endl
       << "Path: " << other.path() << std::endl;

    if (!other.isLogical)
      os << "Disk path: " << other.tablePath() << std::endl
         << "Index: " << other.index() << std::endl
         << "GUID: " << other.gptPart.GetUniqueGUID().AsString() << std::endl;

    return os;
  }
}; // class BasicPartition_t

using Partition_t = BasicPartition_t<uint32_t, uint64_t, std::filesystem::path>;

static_assert(IsValidPartitionClass<Partition_t>, "BasicPartition_t is doesn't meet requirements of minimumPartitionClass");

inline std::error_code make_error_code(Errors ec) { return {static_cast<int>(ec), Partition_t::getErrorCategory()}; }

struct Progress_t {
  const std::string name;
  const uint64_t total;
  std::atomic<uint64_t> done{0};
  std::atomic<bool> finished{false};
  std::atomic<bool> failed{false};

  Progress_t() = delete;
  Progress_t(std::string name, uint64_t total) : name(std::move(name)), total(total) {}

  Progress_t(const Progress_t &) = delete;
  Progress_t &operator=(const Progress_t &) = delete;
};

class ProgressRenderer {
  std::vector<std::shared_ptr<Progress_t>> _entries;
  std::thread _thread;
  std::atomic<bool> _running{false};
  std::mutex _mutex;
  size_t _drawnCount = 0;

  void render() {
    while (_running.load(std::memory_order_relaxed)) {
      draw();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  void draw() {
    std::lock_guard lock(_mutex);
    const size_t count = _entries.size();
    if (count == 0) return;

    if (_drawnCount > 0) std::cout << "\033[" << _drawnCount << "A";
    _drawnCount = 0;

    for (const auto &p : _entries) {
      const uint64_t done = p->done.load(std::memory_order_relaxed);
      const uint64_t total = p->total;

      if (p->failed.load(std::memory_order_relaxed)) {
        std::cout << "\033[2K\r\n";
        _drawnCount++;
        continue;
      }

      const float pct = total > 0 ? static_cast<float>(done) / static_cast<float>(total) : 0.0f;
      const int filled = static_cast<int>(pct * 20.0f);

      std::string bar;
      bar.reserve(20 * 3);
      for (int i = 0; i < filled; i++)
        bar += "━";
      for (int i = filled; i < 20; i++)
        bar += "╌";

      std::cout << std::left << std::setw(16) << p->name << " [" << bar << "] " << std::right << std::setw(3)
                << static_cast<int>(pct * 100) << "%"
                << "\033[K"
                << "\r\n";
      _drawnCount++;
    }

    std::cout.flush();
  }

public:
  ~ProgressRenderer() { stop(); }

  std::shared_ptr<Progress_t> add(const std::string &name, uint64_t total) {
    std::lock_guard lock(_mutex);
    auto p = std::make_shared<Progress_t>(name, total);
    _entries.push_back(p);
    return p;
  }

  void start() {
    {
      std::lock_guard lock(_mutex);
      for (size_t i = 0; i < _entries.size(); i++)
        std::cout << "\n";
      std::cout.flush();
    }
    _running.store(true, std::memory_order_relaxed);
    _thread = std::thread(&ProgressRenderer::render, this);
  }

  void stop() {
    _running.store(false, std::memory_order_relaxed);
    if (_thread.joinable()) _thread.join();
    draw();
  }

  ProgressRenderer() = default;
  ProgressRenderer(const ProgressRenderer &) = delete;
  ProgressRenderer &operator=(const ProgressRenderer &) = delete;
};

} // namespace PartitionMap

#endif // #ifndef LIBPARTITION_MAP_PARTITION_HPP
