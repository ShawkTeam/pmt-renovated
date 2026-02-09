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

#ifndef LIBPARTITION_MAP_LIB_HPP
#define LIBPARTITION_MAP_LIB_HPP

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <libhelper/lib.hpp>
#include <gpt.h>
#ifdef NONE
#undef NONE
#endif

namespace PartitionMap {

enum SizeUnit : int { BYTE = 1, KiB = 2, MiB = 3, GiB = 4 };

struct BasicData {
  GPTPart gptPart;
  uint32_t index;
  std::filesystem::path tablePath;
};
struct BasicInfo {
  std::string name;
  uint64_t size;
  bool isLogical;
};

template <typename __class>
concept minimumPartitionClass = requires(__class cls, __class cls2, GUIDData gdata, SizeUnit unit, uint32_t sector, bool no_throw,
                                         const BasicData &data, const std::filesystem::path &path) {
  // Check required functions
  { cls.getPath() } -> std::same_as<std::filesystem::path>;
  { cls.getPathByName() } -> std::same_as<std::filesystem::path>;
  { cls.getAbsolutePath() } -> std::same_as<std::filesystem::path>;
  { cls.getName() } -> std::convertible_to<std::string>;
  { cls.getFormattedSizeString(unit, no_throw) } -> std::convertible_to<std::string>;
  { cls.getSize(sector) } -> std::same_as<uint64_t>;
  { cls.empty() } -> std::convertible_to<bool>;

  // Check required constructors
  __class{};
  ~__class{};
  __class{path};
  __class{data};
  __class{cls};
  __class(std::move(cls2));

  // Check required operators
  { cls == cls2 } -> std::convertible_to<bool>;
  { cls == gdata } -> std::convertible_to<bool>;
  { cls != cls2 } -> std::convertible_to<bool>;
  { cls != gdata } -> std::convertible_to<bool>;
  { static_cast<bool>(cls) } -> std::convertible_to<bool>;
  { !cls } -> std::convertible_to<bool>;
  cls = cls2;
  cls = std::move(cls2);
}; // concept minimumPartitionClass

class Partition_t {
  std::filesystem::path tablePath;            // The table path to which the partition
                                              // belongs (like /dev/block/sdc).
  std::filesystem::path logicalPartitionPath; // Path of logical partition.
  uint32_t index = 0;                         // The actual index of the partition within the table.
  mutable GPTPart gptPart;                    // Complete data for the partition.

  bool isLogical = false; // This class contains a logical partition?

public:
  class Extra {
  public:
    static bool isReallyLogical(const std::filesystem::path &path);
  };

  using BasicData = PartitionMap::BasicData;

  static Partition_t &AsLogicalPartition(Partition_t &orig, const std::filesystem::path &path);

  Partition_t() : gptPart(GPTPart()) {}            // Partition_t partititon
  Partition_t(const Partition_t &other) = default; // Partition_t partition(otherPartition)
  Partition_t(Partition_t &&other) noexcept
      : tablePath(std::move(other.tablePath)), logicalPartitionPath(std::move(other.logicalPartitionPath)), index(other.index),
        gptPart(other.gptPart), isLogical(other.isLogical) { // Partition_t partition(std::move(otherPartition))
    other.index = 0;
    other.gptPart = GPTPart();
    other.isLogical = false;
  }
  explicit Partition_t(const BasicData &input)
      : tablePath(input.tablePath), index(input.index), gptPart(input.gptPart) {
  } // Partition_t partition({myGptPart, 4, "/dev/block/sda"}); For normal partitions.
  explicit Partition_t(const std::filesystem::path &path) /* NOLINT(modernize-pass-by-value) */
      : logicalPartitionPath(path), gptPart(GPTPart()), isLogical(true) {
  } // Partition_t logicalPartition("/dev/block/mapper/system"); For logical partitions.

  GPTPart getGPTPart() const;           // Get copy of GPTPart data.
  GPTPart *getGPTPartRef();             // Get reference of GPTPart data (non-const reference).
  const GPTPart *getGPTPartRef() const; // Get reference of GPTPart data (const reference).

  std::filesystem::path getPath() const; // Get partition path (like /dev/block/sdc4)
  std::filesystem::path
  getAbsolutePath() const; // Get absolute partition path (returns std::filesystem::read_symlink(getPath()) for normal partitions)
  const std::filesystem::path &getTablePath() const; // Get tablePath variable (const reference).
  std::filesystem::path &getTablePath();             // Get tablePath variable (non-const reference).
  std::filesystem::path getPathByName() const;       // Get partition path by name.

  std::string getName() const;      // Get partition name.
  std::string getTableName() const; // Get table name.
  std::string getFormattedSizeString(SizeUnit size_unit,
                                     bool no_type = false) const; // Get partition size as formatted string.

  std::string getGUIDAsString() const; // Get partition GUID as string.

  const uint32_t &getIndex() const;                        // Get partition index in GPT table (const).
  uint32_t &getIndex();                                    // Get partition index in GPT table (non-const).
  uint64_t getSize(uint32_t sectorSize = 4096) const;      // Get partition size in bytes.
  uint64_t getStartByte(uint32_t sectorSize = 4096) const; // Starting byte address.
  uint64_t getEndByte(uint32_t sectorSize = 4096) const;   // Ending byte address.

  GUIDData getGUID() const; // Get partition GUID.

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

  Partition_t &operator=(const Partition_t &other) = default;            // p1 = p2
  Partition_t &operator=(Partition_t &&other) noexcept;                  // p1 = std::move(p2)
  friend std::ostream &operator<<(std::ostream &os, Partition_t &other); // std::cout << p1
}; // class Partition_t

// template <typename __class>
// requires minimumPartitionClass<__class>
class Builder {
  std::vector<Partition_t> partitions;
  std::map<std::filesystem::path, std::shared_ptr<GPTData>> gptDataCollection;
  std::unordered_set<std::string> tableNames;

  bool buildAutoOnDiskChanges, isUFS;
  std::string seek;

  void scan();
  void scanLogicalPartitions();
  void findTablePaths();

public:
  class Extra {
  public:
    static bool isReallyTable(const std::string &name);
  };

  Builder() : buildAutoOnDiskChanges(true), isUFS(false) { // Builder map
    findTablePaths();
    scan();
    scanLogicalPartitions();
  }

  Builder(Builder &&other) noexcept
      : partitions(std::move(other.partitions)), gptDataCollection(std::move(other.gptDataCollection)),
        tableNames(std::move(other.tableNames)), buildAutoOnDiskChanges(other.buildAutoOnDiskChanges), isUFS(other.isUFS),
        seek(std::move(other.seek)) { // Builder map(std::move(otherMap))
    other.buildAutoOnDiskChanges = true;
    other.isUFS = false;
  }

  std::vector<Partition_t *> getAllPartitions(); // Get references of all partitions (std::vector<Partition_t*>, non-const).
  std::vector<const Partition_t *> getAllPartitions() const; // Get references of all partitions (std::vector<Partition_t*>, const).
  std::vector<Partition_t *> getPartitions();                // Get references of partitions (std::vector<Partition_t*>, non-const).
  std::vector<const Partition_t *> getPartitions() const;    // Get references of partitions (std::vector<const Partition_t*>, const).
  std::vector<Partition_t *> getLogicalPartitions();         // Get references of logical partitions (non-const).
  std::vector<const Partition_t *> getLogicalPartitions() const; // Get references of logical partitions (const).
  std::vector<Partition_t *>
  getPartitionsByTable(const std::string &name); // Get references of partitions of table by name (non-const).
  std::vector<const Partition_t *>
  getPartitionsByTable(const std::string &name) const; // Get references of partitions of table by name (const).

  std::vector<std::pair<bool, std::string>> getDuplicatePartitionPositions(const std::string &name) const;

  const std::unordered_set<std::string> &getTableNames() const;    // Get references of table names (const)
  std::unordered_set<std::string> &getTableNames();                // Get references of table names (non-const).
  std::unordered_set<std::filesystem::path> getTablePaths() const; // Get table paths (form gptDataCollection).

  const std::map<std::filesystem::path, std::shared_ptr<GPTData>> &getAllGPTData() const; // Get gptDataCollection.

  const std::shared_ptr<GPTData> &getGPTDataOf(
      const std::string &name) const; // Get gpt data of table by name (retrieve it as const std::shared_ptr as it is stored, const).
  std::shared_ptr<GPTData> &
  getGPTDataOf(const std::string &name); // Get gpt data of table by name (retrieve it as std::shared_ptr as it is stored, non-const).

  std::vector<std::pair<std::string, uint64_t>> getDataOfLogicalPartitions(); // Get logical partition information.
  std::vector<BasicInfo> getDataOfPartitions();                               // Get information about partitions.
  std::vector<BasicInfo> getDataOfPartitionsByTable(const std::string &name); // Get information about partitions by table name.

  const Partition_t &partition(const std::string &name, const std::string &from = "") const;
  Partition_t &partition(const std::string &name, const std::string &from = "");
  const Partition_t &partitionWithDupCheck(const std::string &name, bool check = true) const;
  Partition_t &partitionWithDupCheck(const std::string &name, bool check = true);

  const std::string &getSeek() const;
  std::string &getSeek(); // Get the table name that the [uint32_t] operator will use.

  int hasDuplicateNamedPartition(const std::string &name) const;
  int hasDuplicateNamedPartition(const std::string &name); // Check <name> named partitions are duplicate.

  bool
  hasPartition(const std::string &name) const; // Check <name> partition is existing or not (checks partitions and logicalPartitions).
  bool hasLogicalPartition(const std::string &name) const; // Check <name> logical partition is existing or not.
  bool hasTable(const std::string &name) const;            // Check <name> table name is existing or not.
  bool isUsesUFS() const;                                  // Get the device uses UFS or not.
  bool isHasSuperPartition() const;                        // Get any table has super partition or not.
  bool isLogical(const std::string &name) const;           // Check <name> is logical partition or
                                                           // not. Same as hasLogicalPartition().

  bool empty() const; // Checks partitions, logicalPartitions and
                      // gptDataCollection is empty or not.

  bool tableNamesEmpty() const; // Checks tableNames is empty or not.

  bool valid() const; // Validate GPTData collection integrity. Checks with
                      // GPTData::Verify() and GPTData::CheckHeaderValidity().

  bool foreach (const std::function<bool(const Partition_t &)> &function) const; // For-each input function for all partitions (const).
  bool foreach (const std::function<bool(Partition_t &)> &function); // For-each input function for all partitions (non-const).
  bool foreachPartitions(const std::function<bool(const Partition_t &)> &function) const; // For-each input function for partitions.
  bool foreachPartitions(const std::function<bool(Partition_t &)> &function);             // For-each input function for partitions.
  bool foreachLogicalPartitions(
      const std::function<bool(const Partition_t &)> &function) const;               // For-each input function for logical partitions.
  bool foreachLogicalPartitions(const std::function<bool(Partition_t &)> &function); // For-each input function for logical partitions.
  bool foreachGptData(
      const std::function<bool(const std::filesystem::path &,
                               const std::shared_ptr<GPTData> &)> &function) const; // For-each input function for gpt data collection.
  bool
  foreachGptData(const std::function<bool(const std::filesystem::path &,
                                          std::shared_ptr<GPTData> &)> &function); // For-each input function for gpt data collection.
  bool foreachFor(
      const std::vector<std::string> &list,
      const std::function<bool(const Partition_t &)> &function) const; // For-each input function for input partition list (all types).
  bool foreachFor(const std::vector<std::string> &list,
                  const std::function<bool(Partition_t &)> &function); // For-each input function for input partition list (all types).
  bool foreachForLogicalPartitions(const std::vector<std::string> &list,
                                   const std::function<bool(const Partition_t &)> &function)
      const; // For-each input function for input partition list (only normal partitions).
  bool foreachForLogicalPartitions(const std::vector<std::string> &list,
                                   const std::function<bool(Partition_t &)>
                                       &function); // For-each input function for input partition list (only normal partitions).
  bool foreachForPartitions(const std::vector<std::string> &list,
                            const std::function<bool(const Partition_t &)> &function)
      const; // For-each input function for input partition list (only normal partitions).
  bool foreachForPartitions(const std::vector<std::string> &list,
                            const std::function<bool(Partition_t &)>
                                &function); // For-each input function for input partition list (only normal partitions).

  void reScan(bool auto_toggle = false);     // Rescan tables.
  void addTable(const std::string &name);    // Add table.
  void removeTable(const std::string &name); // Remove table.
  void setSeek(const std::string &name);     // Set the table name that the [uint32_t] operator will use.

  void setTables(std::unordered_set<std::string> names); // Set tables. By names (like sda, sdc, sdg).
  void setGPTDataOf(const std::string &name,
                    std::shared_ptr<GPTData> data); // Set GPTData of <name> table.
  void setAutoScanOnTableChanges(bool state);       // Set auto scanning as <state>.
  void clear();                                     // Cleanup data (excepts auto scan state and seek name).
  void reset();                                     // Cleanup (clear()) and reset variables.

  bool operator==(const Builder &other) const; // pd1 == pd2
  bool operator!=(const Builder &other) const; // pd1 != pd2
  explicit operator bool() const;              // if (pd) { ... } (equals to valid())
  bool operator!() const;                      // if (!pd) { ... } (equals to !valid())

  const std::shared_ptr<GPTData> &operator[](const std::string &name) const; // std::shared_ptr<GPTData> data = pd["sda"]
  std::shared_ptr<GPTData> &operator[](const std::string &name);             // std::shared_ptr<GPTData> data = pd["sda"]
  const GPTPart *operator[](uint32_t index) const;                           // GPTPart part = pd[2]
  GPTPart *operator[](uint32_t index);                                       // GPTPart part = pd[2]

  Builder &operator=(const Builder &other) = default; // pd2 = pd1
  Builder &operator=(Builder &&other) noexcept;       // pd2 = std::move(p1)
};

using Error = Helper::Error;

constexpr int NAME = 0;
constexpr int SIZE = 1;
constexpr int DYNAMIC = 2;

std::string getLibVersion();

namespace Extra {
namespace FileSystemMagic {
constexpr uint64_t EXTFS_FS = 0xEF53;
constexpr uint64_t F2FS_FS = 0xF2F52010;
constexpr uint64_t EROFS_FS = 0xE0F5E1E2;
constexpr uint64_t EXFAT_FS = 0x5441465845;
constexpr uint64_t FAT12_FS = 0x3231544146;
constexpr uint64_t FAT16_FS = 0x3631544146;
constexpr uint64_t FAT32_FS = 0x3233544146;
constexpr uint64_t NTFS_FS = 0x5346544E;
constexpr uint64_t MSDOS_FS = 0x4d44;
} // namespace FileSystemMagic

namespace AndroidMagic {
constexpr uint64_t BOOT_IMAGE = 0x2144494F52444E41;
constexpr uint64_t VBOOT_IMAGE = 0x544F4F4252444E56;
constexpr uint64_t LK_IMAGE = 0x00006B6C;
constexpr uint64_t DTBO_IMAGE = 0x1EABB7D7;
constexpr uint64_t VBMETA_IMAGE = 0x425641;
constexpr uint64_t SUPER_IMAGE = 0x61446C67;
constexpr uint64_t SPARSE_IMAGE = 0x3AFF26ED;
constexpr uint64_t ELF = 0x464C457F;
constexpr uint64_t RAW = 0x00000000;
} // namespace AndroidMagic

extern std::map<uint64_t, std::string> FileSystemMagics;
extern std::map<uint64_t, std::string> AndroidMagics;
extern std::map<uint64_t, std::string> Magics;

size_t getMagicLength(uint64_t magic);
bool hasMagic(uint64_t magic, ssize_t buf, const std::string &path);
std::string formatMagic(uint64_t magic);
std::string getSizeUnitAsString(SizeUnit size);
} // namespace Extra
} // namespace PartitionMap

// clang-format off
#define FOREACH_PARTITIONS_LAMBDA_PARAMETERS         (PartitionMap::Partition_t & partition)
#define FOREACH_PARTITIONS_LAMBDA_PARAMETERS_CONST   (const PartitionMap::Partition_t & partition)
#define FOREACH_GPT_DATA_LAMBDA_PARAMETERS           (const std::filesystem::path &path, std::shared_ptr<GPTData> &gptData)
#define FOREACH_GPT_DATA_LAMBDA_PARAMETERS_CONST     (const std::filesystem::path &path, const std::shared_ptr<GPTData> &gptData)
// clang-format on

#endif // #ifndef LIBPARTITION_MAP_LIB_HPP
