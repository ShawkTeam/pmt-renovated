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

/**
 * @file partition.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Provides partition management class.
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
#include <libopenpart/openpart.h>
#include <libpartition_map/definations.hpp>

/**
 * @brief Basic partition management class.
 *
 * @tparam slot_type Slot type for holding partition index.
 * @tparam size_type Size type for holding partition size.
 * @tparam path_type Path type for holding partition path.
 */
namespace PartitionMap {
template <typename slot_type, typename size_type, typename path_type>
  requires IsSlotType<slot_type> && IsSizeType<size_type> && IsPathTypeLike<path_type>
class BasicPartition_t {
  path_type localTablePath;       // The table path to which the partition belongs (like /dev/block/sdc).
  path_type logicalPartitionPath; // Path of logical partition.
  slot_type localIndex = 0;       // The actual index of the partition within the table.
  mutable GPTPart gptPart;        // Complete data for the partition.
  openpart_t *op;                 // libopenpart object.

  bool isLogical = false; // This class contains a logical partition?

  void process_ctor(const slot_type &index) { localIndex = index; }
  void process_ctor(const GPTPart &part) { gptPart = part; }
  void process_ctor(const path_type &path) { localTablePath = path; }
  void process_ctor(openpart_t *_op) { op = _op; }

public:
  /// @brief Extra functions for partition management.
  class Extra {
  public:
    /// @brief Checks if the path is a logical partition.
    static bool isReallyLogical(const path_type &path) { return path.string().find("/mapper/") != std::string::npos; }
  };

  using BasicData = basic_data_base<slot_type>; ///< Short for @c basic_data_base.

  /// @note First arg = written/readed size, second arg = total size.
  using IOCallback = std::function<void(size_type, size_type)>;

  /// @brief Get object for logical partition.
  static BasicPartition_t &AsLogicalPartition(BasicPartition_t &orig, const path_type &path) {
    orig.isLogical = true;
    orig.logicalPartitionPath = path;
    orig.gptPart = GPTPart();

    openpart_close(&orig.op);
    orig.op = openpart_open(path.string().c_str(), OP_RDWR, 0);
    if (!orig.op) throw Error("Cannot create openpart object: {}", openpart_strerror(orig.op));

    return orig;
  }

  /// @brief Destructor.
  ~BasicPartition_t() { openpart_close(&op); }

  /// @brief Default constructor.
  BasicPartition_t() : gptPart(GPTPart()), op(nullptr) {}
  /// @brief Copy constructor.
  BasicPartition_t(const BasicPartition_t &other)
      : localTablePath(other.localTablePath), logicalPartitionPath(other.logicalPartitionPath), localIndex(other.localIndex),
        gptPart(other.gptPart), isLogical(other.isLogical) {
    if (other.op) {
      std::string path = other.isLogical ? other.logicalPartitionPath.string() : std::string(openpart_get_part_path(other.op));
      op = openpart_open(path.c_str(), OP_RDWR, 0);
      if (!op) throw Error("Cannot create openpart object: {}", openpart_strerror(op));
    } else
      op = nullptr;
  }
  /// @brief Move constructor.
  BasicPartition_t(BasicPartition_t &&other) noexcept
      : localTablePath(std::move(other.localTablePath)), logicalPartitionPath(std::move(other.logicalPartitionPath)),
        localIndex(other.localIndex), gptPart(other.gptPart), op(other.op), isLogical(other.isLogical) {
    other.localIndex = 0;
    other.gptPart = GPTPart();
    other.op = nullptr;
    other.isLogical = false;
  }

  /**
   * @brief Constructor from arguments.
   *
   * @code
   * BasicPartition_t<uint32_t, uint64_t, std::filesystem::path> partition(myGptPart, openPartPtr, "/dev/block/sda", 4);
   * // For normal partitions! The order of arguments may differ from the example.
   * @endcode
   *
   * @tparam Args Arguments.
   */
  template <typename... Args>
    requires(sizeof...(Args) == 4) && FindInArgs::HasSlotType<Args...> || FindInArgs::HasStringOrPath<Args...> ||
                FindInArgs::HasGPTPart<Args...> || FindInArgs::HasNonConstOpenPartPtr<Args...>
            explicit BasicPartition_t(Args &&...args) {
    (process_ctor(std::forward<Args>(args)), ...);
  }

  /**
   * @brief Constructor from basic data.
   *
   * @code
   * BasicPartition_t<uint32_t, uint64_t, std::filesystem::path> partition({myGptPart, openPartPtr, 4, "/dev/block/sda"});
   * // Only for normal partitions.
   * @endcode
   *
   * @param input Basic data.
   */
  explicit BasicPartition_t(const basic_data_base<slot_type> &input)
      : localTablePath(input.tablePath), localIndex(input.index), gptPart(input.gptPart) {
    if (input.op) {
      const char *path = openpart_get_part_path(input.op);
      op = openpart_open(path, OP_RDWR, 0);
      if (!op) throw Error("Cannot create openpart object: {}", openpart_strerror(op));
    } else
      op = nullptr;
  }

  /**
   * @brief Constructor for logical partitions.
   *
   * @code
   * BasicPartition_t<uint32_t, uint64_t, std::filesystem::path> logicalPartition("/dev/block/mapper/system");
   * // Only for logical partitions.
   * @endcode
   *
   * @param path Logical partition path.
   */
  explicit BasicPartition_t(const path_type &path) /* NOLINT(modernize-pass-by-value) */
      : logicalPartitionPath(path), gptPart(GPTPart()), isLogical(true) {
    op = openpart_open(path.string().c_str(), OP_RDWR, 0);
    if (!op) throw Error("Cannot create openpart object: {}", openpart_strerror(op));
  }

  /// @brief Get copy of @c GPTPart data.
  GPTPart getGPTPart() const {
    if (isLogical) throw Error("Cannot get GPTPart data: Is logical partition");
    return gptPart;
  }

  /// @brief Get reference of @c GPTPart data (non-constant reference).
  GPTPart *getGPTPartRef() {
    if (isLogical) throw Error("Cannot get GPTPart data: Is logical partition");
    return &gptPart;
  }

  /// @brief Get reference of @c GPTPart data (constant reference).
  const GPTPart *getGPTPartRef() const {
    if (isLogical) throw Error("Cannot get GPTPart data: Is logical partition");
    return &gptPart;
  }

  /// @brief Get openpart object.
  openpart_t *getOpenPart() { return op; }

  /// @brief Get openpart object (constant reference).
  const openpart_t *getOpenPart() const { return op; }

  /// @brief Get partition path (like @c /dev/block/sdc4 ).
  path_type path() const {
    const std::string suffix = isdigit(localTablePath.string().back()) ? "p" : "";
    path_type path = isLogical ? logicalPartitionPath : path_type(localTablePath.string() + suffix + std::to_string(localIndex + 1));
    return path;
  }

  /**
   * @brief Get absolute partition path.
   * @return std::filesystem::read_symlink(getPath()) For normal partitions.
   */
  path_type absolutePath() const {
    if (isLogical) return std::filesystem::read_symlink(logicalPartitionPath);
    return path();
  }

  /// @brief Get @c tablePath variable.
  path_type tablePath() const {
    if (isLogical) throw Error("Cannot return table path: Is logical partition");
    return localTablePath;
  }

  /// @brief Get partition path by name.
  path_type pathByName() const {
    if (isLogical) return logicalPartitionPath;
    path_type result = "/dev/block/by-name";
    result.append(gptPart.GetDescription());

    if (!std::filesystem::exists("/dev/block/by-name") || std::filesystem::read_symlink(result) != path()) return {};
    return result;
  }

  /// @brief Get partition name.
  std::string name() const {
    if (isLogical) return logicalPartitionPath.filename().string();
    return gptPart.GetDescription();
  }

  /// @brief Get table name.
  std::string tableName() const {
    if (isLogical) throw Error("Cannot return table name: Is logical partition");
    return localTablePath.filename().string();
  }

  /// @brief Get partition size as formatted string.
  std::string formattedSizeString(const SizeUnit size_unit, bool no_type = false) const {
    size_type size_ = size();
    double calculated_size = static_cast<double>(size_);
    std::string unit_str = "B";

    switch (size_unit) {
      case BYTE:
        unit_str = "B";
        break;
      case KiB:
        calculated_size = static_cast<double>(size_) / 1024.0;
        unit_str = "KiB";
        break;
      case MiB:
        calculated_size = static_cast<double>(size_) / (1024.0 * 1024.0);
        unit_str = "MiB";
        break;
      case GiB:
        calculated_size = static_cast<double>(size_) / (1024.0 * 1024.0 * 1024.0);
        unit_str = "GiB";
        break;
    }

    std::stringstream ss;
    if (size_unit == BYTE)
      ss << size_;
    else
      ss << std::fixed << std::setprecision(2) << calculated_size;
    if (!no_type) ss << unit_str;

    return ss.str();
  }

  /// @brief Get partition GUID as string.
  std::string GUIDAsString() const {
    if (isLogical) throw Error("Cannot return table name: Is logical partition");
    return gptPart.GetUniqueGUID().AsString();
  }

  /// @brief Get partition index in GPT table.
  const slot_type index() const {
    if (isLogical) throw Error("Cannot return index: Is logical partition");
    return localIndex;
  }

  /// @brief Get partition size in bytes.
  size_type size() const {
    uint64_t size = openpart_get_size(op);
    if (size == UINT64_MAX)
      throw Error("Cannot get size of {}: {}", name(), openpart_strerror(op));
    return size;
  }

  /// @brief Get starting byte address.
  size_type start() const {
    if (isLogical) throw Error("Cannot return start address: Is logical partition");
    return gptPart.GetFirstLBA() * openpart_get_sector_size(op);
  }

  /// @brief Get ending byte address.
  size_type end() const {
    if (isLogical) throw Error("Cannot return end address: Is logical partition");
    return (gptPart.GetLastLBA() + 1) * openpart_get_sector_size(op);
  }

  /// @brief Get partition GUID.
  GUIDData GUID() const {
    if (isLogical) throw Error("Cannot return GPTData GUID: Is logical partition");
    return gptPart.GetUniqueGUID();
  }

  /// @brief Dump image of partition.
  [[maybe_unused]] bool dump(const path_type &destination = "", size_type bufsize = MB(1), IOCallback callback = nullptr) const {
    const path_type dest = destination.empty() ? (path_type("./") += name() + ".img") : destination;
    const path_type toOpen = isLogical ? absolutePath() : path();

    if (!op) throw Error("openpart_t* object invalid: {}", std::quoted_string(toOpen.c_str()));

    auto outfd = Helper::UniqueFD(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (!outfd) throw Error("Cannot create/open {}: {}", dest.string(), strerror(errno));

    const size_type bufferSize = std::min<size_type>(bufsize, size());
    std::vector<char> buffer(bufferSize);

    const size_type totalBytesToRead = size();
    size_type bytesReadSoFar = 0;

    while (bytesReadSoFar < totalBytesToRead) {
      size_type toRead = std::min(bufferSize, totalBytesToRead - bytesReadSoFar);

      ssize_t bytesRead = openpart_read(op, buffer.data(), toRead, bytesReadSoFar);
      if (bytesRead <= 0) {
        if (errno == EPERM) throw Error("Cannot read {}: {}", toOpen.string(), strerror(errno));
      }

      if (const ssize_t bytesWritten = outfd.write(buffer.data(), bytesRead); bytesWritten != bytesRead)
        throw Error("Cannot write {}: {}", dest.string(), strerror(errno));

      bytesReadSoFar += bytesRead;
      if (callback) callback(bytesReadSoFar, totalBytesToRead);
    }

    return bytesReadSoFar == totalBytesToRead;
  }

  /// @brief Write input image to partition.
  [[maybe_unused]] bool write(const path_type &image, size_type bufsize = MB(1), IOCallback callback = nullptr) {
    const path_type toWrite = isLogical ? absolutePath() : path();
    const int64_t imageSize = Helper::fileSize(image);
    if (imageSize < 0) throw Error("Cannot get size of {}: {}", image.string(), strerror(errno));
    if (imageSize > size()) throw Error("Image is too large: {} ({} > {})", image.string(), imageSize, size());

    if (!op) throw Error("openpart_t* object invalid: {}", std::quoted_string(toWrite.c_str()));

    auto imagefd = Helper::UniqueFD(image, O_RDONLY);
    if (!imagefd) throw Error("Cannot open {}: {}", image.string(), strerror(errno));

    size_type bytesWrittenSoFar = 0;
    const size_type bufferSize = std::min<size_type>(bufsize, size());
    std::vector<char> buffer(bufferSize);

    while (bytesWrittenSoFar < imageSize) {
      size_type toRead = std::min<size_type>(bufferSize, imageSize - bytesWrittenSoFar);

      ssize_t bytesRead = imagefd.read(buffer.data(), toRead);
      if (bytesRead <= 0) throw Error("Cannot read {}: {}", image.string(), strerror(errno));

      if (const ssize_t bytesWritten = openpart_write(op, buffer.data(), bytesRead, bytesWrittenSoFar); bytesWritten != bytesRead)
        throw Error("Cannot write {}: {}", toWrite.string(), strerror(errno));

      bytesWrittenSoFar += bytesRead;
      if (callback) callback(bytesWrittenSoFar, imageSize);
    }

    if (bytesWrittenSoFar < size()) {
      size_type remainingBytes = size() - bytesWrittenSoFar;
      std::ranges::fill(buffer, 0x00);

      while (remainingBytes > 0) {
        size_type toWriteSize = std::min<uint64_t>(buffer.size(), remainingBytes);
        ssize_t written = openpart_write(op, buffer.data(), toWriteSize, bytesWrittenSoFar);

        if (written <= 0) throw Error("Cannot fill the outside of partition (of image): {}", strerror(errno));
        remainingBytes -= written;
        bytesWrittenSoFar += written;
      }
    }

    Log::info("Syncing {}...", toWrite.string());
    openpart_sync(op);
    return bytesWrittenSoFar == imageSize;
  }

  /// @brief Set @c GPTPart object, index and table path.
  void set(const basic_data_base<slot_type> &data) {
    if (isLogical) throw Error("This is not a normal partition object!");
    gptPart = data.gptPart;
    op = data.op;
    localTablePath = data.tablePath;
    localIndex = data.index;
  }

  /// @brief Set partition path. Only for logical partitions.
  void setPartitionPath(const path_type &path) {
    if (!isLogical) throw Error("This is not a logical partition object!");
    logicalPartitionPath = path;
  }

  /// @brief Set partition index.
  void setIndex(slot_type new_index) {
    if (isLogical) throw Error("This is not a normal partition object!");
    localIndex = new_index;
  }

  /// @brief Set @c GPTPart object.
  void setGptPart(const GPTPart &otherGptPart) {
    if (isLogical) throw Error("This is not a normal partition object!");
    gptPart = otherGptPart;
  }

  /// @brief Set openpart object. Available openpart_t* objects is releasing.
  void setOpenPart(openpart_t *_op) {
    if (isLogical) throw Error("This is not a normal partition object!");
    openpart_close(&op);
    op = _op;
  }

  /// @brief Checks whether the partition is dynamic or not.
  bool isSuperPartition() const {
    if (isLogical) throw Error("This is not a normal partition object!");
    return GUID() == GUIDData("89A12DE1-5E41-4CB3-8B4C-B1441EB5DA38");
  }

  /// @brief Checks whether the partition is logical or not.
  bool isLogicalPartition() const { return isLogical; }

  /// @brief Checks whether the partition info is empty or not.
  bool empty() const { return isLogical ? logicalPartitionPath.empty() : !gptPart.IsUsed() && localTablePath.empty(); }

  /// @brief Closes the file descriptor.
  bool closeFdNow() { return close(openpart_get_fd(op)) == 0; }

  /**
   * @name @c BasicPartition_t's operators.
   * @brief Operators of @c BasicPartition_t class.
   * @{
   */

  /// @brief Checks whether two partitions are equal and have the same openpart_t pointers.
  bool areSame(const BasicPartition_t &other) const { return *this == other && op == other.op; }

  /// @brief Checks whether two partitions are equal.
  bool operator==(const BasicPartition_t &other) const {
    if (isLogical) return logicalPartitionPath == other.logicalPartitionPath;
    return localTablePath == other.localTablePath && localIndex == other.localIndex &&
           gptPart.GetUniqueGUID() == other.gptPart.GetUniqueGUID();
  }

  /// @brief Checks whether the partition is equal to the given GUID.
  bool operator==(const GUIDData &other) const {
    if (isLogical) throw Error("This is not a normal partition object!");
    return gptPart.GetUniqueGUID() == other;
  }

  bool operator!=(const BasicPartition_t &other) const { return *this != other; } ///< Checks whether two partitions are not equal.
  bool operator!=(const GUIDData &other) const {
    return *this != other;
  } ///< Checks whether the partition is not equal to the given GUID.

  explicit operator bool() const { return !empty(); } ///< It indicates whether the information is not empty.
  bool operator!() const { return empty(); }          ///< It indicates whether the information is empty.

  const GPTPart *operator*() const { return getGPTPart(); } ///< Get @c GPTPart object (as const).
  GPTPart *operator*() { return getGPTPart(); }             ///< Get @c GPTPart object.

  /// @brief Copy assignment operator.
  BasicPartition_t &operator=(const BasicPartition_t &other) {
    if (this != &other) {
      localTablePath = other.localTablePath;
      logicalPartitionPath = other.logicalPartitionPath;
      localIndex = other.localIndex;
      gptPart = other.gptPart;
      isLogical = other.isLogical;

      if (op) {
        openpart_close(&op);
        op = nullptr;
      }
      if (other.op) {
        std::string path = other.isLogical ? other.logicalPartitionPath.string() : std::string(openpart_get_part_path(other.op));
        op = openpart_open(path.c_str(), O_RDWR, 0);
        if (!op) throw Error("Cannot create openpart object: {}", openpart_strerror(op));
      } else
        op = nullptr;
    }

    return *this;
  }

  /// @brief Move assignment operator.
  BasicPartition_t &operator=(BasicPartition_t &&other) noexcept {
    if (this != &other) {
      localTablePath = std::move(other.localTablePath);
      localIndex = other.localIndex;
      logicalPartitionPath = std::move(other.logicalPartitionPath);
      gptPart = other.gptPart;
      isLogical = other.isLogical;

      if (op) openpart_close(&op);
      op = other.op;
      other.op = nullptr;

      other.localIndex = 0;
      other.gptPart = GPTPart();
      other.isLogical = false;
    }

    return *this;
  }

  /// @brief @c << operator.
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

  /** @} */

}; // class BasicPartition_t

/// @brief Template alias for BasicPartition_t.
using Partition_t = BasicPartition_t<uint32_t, GenericSizeType, std::filesystem::path>;

/// @brief Progress information structure for @c ProgressRenderer.
struct Progress_t {
  using size_type = GenericSizeType;
  const std::string name;            ///< Partition name.
  const size_type total;             ///< Total size.
  std::atomic<size_type> done{0};    ///< Done size.
  std::atomic<bool> finished{false}; ///< Process is finished or not.
  std::atomic<bool> failed{false};   ///< Process is failed or not.

  /// @brief Deleted constructor.
  Progress_t() = delete;
  /// @brief Main constructor.
  Progress_t(std::string name, size_type total) : name(std::move(name)), total(total) {}

  Progress_t(const Progress_t &) = delete;            ///< Deleted copy constructor.
  Progress_t &operator=(const Progress_t &) = delete; ///< Deleted copy assignment.
};

/// @brief Progress renderer class.
class ProgressRenderer {
  std::vector<std::shared_ptr<Progress_t>> _entries;
  std::thread _thread;
  std::atomic<bool> _running{false};
  std::mutex _mutex;
  size_t _drawnCount = 0;

  /// @brief Render loop.
  void render() {
    while (_running.load(std::memory_order_relaxed)) {
      draw();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  /// @brief Draw progress bar.
  void draw() {
    std::lock_guard lock(_mutex);
    const size_t count = _entries.size();
    if (count == 0) return;

    if (_drawnCount > 0) std::cout << "\033[" << _drawnCount << "A";
    _drawnCount = 0;

    for (const auto &p : _entries) {
      const Progress_t::size_type done = p->done.load(std::memory_order_relaxed);
      const Progress_t::size_type total = p->total;

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

  /// @brief Add a new progress entry.
  std::shared_ptr<Progress_t> add(const std::string &name, Progress_t::size_type total) {
    std::lock_guard lock(_mutex);
    auto p = std::make_shared<Progress_t>(name, total);
    _entries.push_back(p);
    return p;
  }

  /// @brief Start the progress renderer.
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

  /// @brief Stop the progress renderer.
  void stop() {
    _running.store(false, std::memory_order_relaxed);
    if (_thread.joinable()) _thread.join();
    draw();
  }

  /// @brief Default constructor.
  ProgressRenderer() = default;
  ProgressRenderer(const ProgressRenderer &) = delete;
  ProgressRenderer &operator=(const ProgressRenderer &) = delete;
}; // class ProgressRenderer

} // namespace PartitionMap

#endif // #ifndef LIBPARTITION_MAP_PARTITION_HPP
