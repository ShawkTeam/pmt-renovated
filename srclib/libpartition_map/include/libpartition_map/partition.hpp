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
#include <gpt.h>
#include <libpartition_map/definations.hpp>
#include <libhelper/macros.hpp>

namespace PartitionMap {
class Partition_t {
  std::filesystem::path localTablePath;       // The table path to which the partition
  // belongs (like /dev/block/sdc).
  std::filesystem::path logicalPartitionPath; // Path of logical partition.
  uint32_t localIndex = 0;                    // The actual index of the partition within the table.
  mutable GPTPart gptPart;                    // Complete data for the partition.

  bool isLogical = false; // This class contains a logical partition?

public:
  class Extra {
  public:
    static bool isReallyLogical(const std::filesystem::path &path);
  };

  enum class Errors {
    Success = 0,
    IsNotLogicalObject = 1,
    IsNotNormalObject = 2,
    CannotOpenLogicalPartition = 3,
    CannotFill = 4,
    IoctlFailed = 5
  };

  class ErrorCategory final : public std::error_category {
  public:
    const char *name() const noexcept override;
    std::string message(int ev) const override;
  };

  static const ErrorCategory &getErrorCategory();

  using BasicData = PartitionMap::BasicData;

  static Partition_t &AsLogicalPartition(Partition_t &orig, const std::filesystem::path &path);

  Partition_t() : gptPart(GPTPart()) {}            // Partition_t partititon
  Partition_t(const Partition_t &other) = default; // Partition_t partition(otherPartition)
  Partition_t(Partition_t &&other) noexcept
      : localTablePath(std::move(other.localTablePath)), logicalPartitionPath(std::move(other.logicalPartitionPath)),
        localIndex(other.localIndex), gptPart(other.gptPart),
        isLogical(other.isLogical) { // Partition_t partition(std::move(otherPartition))
    other.localIndex = 0;
    other.gptPart = GPTPart();
    other.isLogical = false;
  }
  explicit Partition_t(const BasicData &input)
      : localTablePath(input.tablePath), localIndex(input.index), gptPart(input.gptPart) {
  } // Partition_t partition({myGptPart, 4, "/dev/block/sda"}); For normal partitions.
  explicit Partition_t(const std::filesystem::path &path) /* NOLINT(modernize-pass-by-value) */
      : logicalPartitionPath(path), gptPart(GPTPart()), isLogical(true) {
  } // Partition_t logicalPartition("/dev/block/mapper/system"); For logical partitions.

  GPTPart getGPTPart() const;                                       // Get copy of GPTPart data.
  GPTPart getGPTPart(std::error_code &ec) const noexcept;           // Get copy of GPTPart data.
  GPTPart *getGPTPartRef();                                         // Get reference of GPTPart data (non-const reference).
  GPTPart *getGPTPartRef(std::error_code &ec) noexcept;             // Get reference of GPTPart data (non-const reference).
  const GPTPart *getGPTPartRef() const;                             // Get reference of GPTPart data (const reference).
  const GPTPart *getGPTPartRef(std::error_code &ec) const noexcept; // Get reference of GPTPart data (const reference).

  std::filesystem::path path() const;                             // Get partition path (like /dev/block/sdc4).
  std::filesystem::path path(std::error_code &ec) const noexcept; // Get partition path (like /dev/block/sdc4).
  std::filesystem::path
  absolutePath() const; // Get absolute partition path (returns std::filesystem::read_symlink(getPath()) for normal partitions).
  std::filesystem::path absolutePath(std::error_code &ec)
      const noexcept; // Get absolute partition path (returns std::filesystem::read_symlink(getPath()) for normal partitions).
  const std::filesystem::path &tablePath() const;                             // Get tablePath variable (const reference).
  const std::filesystem::path &tablePath(std::error_code &ec) const noexcept; // Get tablePath variable (const reference).
  std::filesystem::path &tablePath();                                         // Get tablePath variable (non-const reference).
  std::filesystem::path &tablePath(std::error_code &ec) noexcept;             // Get tablePath variable (non-const reference).
  std::filesystem::path pathByName() const;                                   // Get partition path by name.
  std::filesystem::path pathByName(std::error_code &ec) const noexcept;       // Get partition path by name.

  std::string name() const;                                  // Get partition name.
  std::string tableName() const;                             // Get table name.
  std::string tableName(std::error_code &ec) const noexcept; // Get table name.
  std::string formattedSizeString(SizeUnit size_unit,
                                  bool no_type = false) const; // Get partition size as formatted string.

  std::string GUIDAsString() const;                             // Get partition GUID as string.
  std::string GUIDAsString(std::error_code &ec) const noexcept; // Get partition GUID as string.

  const uint32_t &index() const;                             // Get partition index in GPT table (const) reference.
  const uint32_t &index(std::error_code &ec) const noexcept; // Get partition index in GPT table (const) reference.
  uint32_t &index();                                         // Get partition index in GPT table (non-const) as reference.
  uint32_t &index(std::error_code &ec) noexcept;             // Get partition index in GPT table (non-const) as reference.
  uint64_t size(uint32_t sectorSize = 4096) const;           // Get partition size in bytes.
  uint64_t start(std::error_code &ec, uint32_t sectorSize = 4096) const noexcept; // Get starting byte address.
  uint64_t start(uint32_t sectorSize = 4096) const;                               // Get starting byte address.
  uint64_t end(std::error_code &ec, uint32_t sectorSize = 4096) const noexcept;   // Get ending byte address.
  uint64_t end(uint32_t sectorSize = 4096) const;                                 // Get ending byte address.

  GUIDData GUID() const;                             // Get partition GUID.
  GUIDData GUID(std::error_code &ec) const noexcept; // Get partition GUID.

  [[maybe_unused]] bool dump(const std::filesystem::path &destination = "",
                             uint64_t bufsize = MB(1)) const; // Dump image of partition.
  [[maybe_unused]] bool dump(std::error_code &ec, const std::filesystem::path &destination = "",
                             uint64_t bufsize = MB(1)) const noexcept; // Dump image of partition.
  [[maybe_unused]] bool write(std::error_code &ec, const std::filesystem::path &image,
                              uint64_t bufsize = MB(1)) noexcept;                            // Write input image to partition.
  [[maybe_unused]] bool write(const std::filesystem::path &image, uint64_t bufsize = MB(1)); // Write input image to partition.

  void set(const BasicData &data);                          // Set GPTPart object, index and table path.
  void setPartitionPath(const std::filesystem::path &path); // Set partition path. Only for logical partitions.
  void setIndex(uint32_t new_index);                        // Set partition index.
  void setDiskPath(const std::filesystem::path &path);      // Set table path.
  void setDiskName(const std::string &name);                // Set table name (automatically adds /dev/block and uses setDiskPath()).
  void setGptPart(const GPTPart &otherGptPart);             // Set GPTPart object.

  bool isSuperPartition() const;   // Checks whether the partition is dynamic or not.
  bool isLogicalPartition() const; // Checks whether the partition is logical or not.
  bool empty() const;              // Checks whether the partition info is empty or not.

  bool operator==(const Partition_t &other) const; // p1 == p2
  bool operator==(const GUIDData &other) const;    // p1 == guid
  bool operator!=(const Partition_t &other) const; // p1 != p2
  bool operator!=(const GUIDData &other) const;    // p1 != guid
  explicit operator bool() const;                  // p (returns !empty())
  bool operator!() const;                          // !p (returns empty())

  const GPTPart *operator*() const;                                      // const GPTPart* part = *p1
  GPTPart *operator*();                                                  // GPTPart* part = *pd1
  Partition_t &operator=(const Partition_t &other) = default;            // p1 = p2
  Partition_t &operator=(Partition_t &&other) noexcept;                  // p1 = std::move(p2)
  friend std::ostream &operator<<(std::ostream &os, Partition_t &other); // std::cout << p1
}; // class Partition_t

static_assert(minimumPartitionClass<Partition_t>, "Partition_t is doesn't meet requirements of minimumPartitionClass");

std::error_code make_error_code(Partition_t::Errors ec);

} // namespace PartitionMap

template <> struct std::is_error_code_enum<PartitionMap::Partition_t::Errors> : true_type {};

#endif // #ifndef LIBPARTITION_MAP_PARTITION_HPP
