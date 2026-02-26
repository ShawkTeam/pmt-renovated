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

#if __cplusplus < 202002L
#error "libpartition_map is requires C++20 or higher C++ standarts."
#endif

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <gpt.h>
/*
#if __ANDROID_API__ >= 30
#include <liblp/liblp.h>
#include <liblp/builder.h>
#include <android-base/properties.h>
#endif
*/

#ifdef NONE
#undef NONE
#endif
#ifdef ERR
#undef ERR
#define ERR PartitionMap::Error()
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
  { cls.path() } -> std::same_as<std::filesystem::path>;
  { cls.pathByName() } -> std::same_as<std::filesystem::path>;
  { cls.absolutePath() } -> std::same_as<std::filesystem::path>;
  { cls.name() } -> std::convertible_to<std::string>;
  { cls.formattedSizeString(unit, no_throw) } -> std::convertible_to<std::string>;
  { cls.size(sector) } -> std::same_as<uint64_t>;
  { cls.empty() } -> std::convertible_to<bool>;

  // Check required constructors
  __class{};
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

// template <typename __class>
// requires minimumPartitionClass<__class>
class Builder {
  std::vector<Partition_t> localPartitions;
  std::map<std::filesystem::path, std::shared_ptr<GPTData>> gptDataCollection;
  std::unordered_set<std::string> localTableNames;

  bool buildAutoOnDiskChanges, isUFS;

  void scan();
  void scanLogicalPartitions();
  void findTablePaths();

public:
  using iterator = std::vector<Partition_t>::iterator;
  using const_iterator = std::vector<Partition_t>::const_iterator;

  class Extra {
  public:
    static bool isReallyTable(const std::string &name);
  };

  Builder() : buildAutoOnDiskChanges(true), isUFS(false) { // Builder map
    findTablePaths();
    scan();
    scanLogicalPartitions();
  }

  Builder(const Builder &other) = default; // Builder map(otherMap)

  Builder(Builder &&other) noexcept
      : localPartitions(std::move(other.localPartitions)), gptDataCollection(std::move(other.gptDataCollection)),
        localTableNames(std::move(other.localTableNames)), buildAutoOnDiskChanges(other.buildAutoOnDiskChanges),
        isUFS(other.isUFS) { // Builder map(std::move(otherMap))
    other.buildAutoOnDiskChanges = true;
    other.isUFS = false;
  }

  std::vector<Partition_t *> allPartitions();             // Get references of all partitions (std::vector<Partition_t*>, non-const).
  std::vector<const Partition_t *> allPartitions() const; // Get references of all partitions (std::vector<Partition_t*>, const).
  std::vector<Partition_t *> partitions();                // Get references of partitions (std::vector<Partition_t*>, non-const).
  std::vector<const Partition_t *> partitions() const;    // Get references of partitions (std::vector<const Partition_t*>, const).
  std::vector<Partition_t *> logicalPartitions();         // Get references of logical partitions (non-const).
  std::vector<const Partition_t *> logicalPartitions() const;            // Get references of logical partitions (const).
  std::vector<Partition_t *> partitionsByTable(const std::string &name); // Get references of partitions of table by name (non-const).
  std::vector<const Partition_t *>
  partitionsByTable(const std::string &name) const; // Get references of partitions of table by name (const).

  std::vector<std::pair<bool, std::string>> duplicatePartitionPositions(const std::string &name) const;

  const std::unordered_set<std::string> &tableNames() const;    // Get references of table names (const)
  std::unordered_set<std::string> &tableNames();                // Get references of table names (non-const).
  std::unordered_set<std::filesystem::path> tablePaths() const; // Get table paths (form gptDataCollection).

  const std::map<std::filesystem::path, std::shared_ptr<GPTData>> &allGPTData() const; // Get gptDataCollection.

  const std::shared_ptr<GPTData> &GPTDataOf(
      const std::string &name) const; // Get gpt data of table by name (retrieve it as const std::shared_ptr as it is stored, const).
  std::shared_ptr<GPTData> &
  GPTDataOf(const std::string &name); // Get gpt data of table by name (retrieve it as std::shared_ptr as it is stored, non-const).

  std::vector<std::pair<std::string, uint64_t>> dataOfLogicalPartitions(); // Get logical partition information.
  std::vector<BasicInfo> dataOfPartitions();                               // Get information about partitions.
  std::vector<BasicInfo> dataOfPartitionsByTable(const std::string &name); // Get information about partitions by table name.

  const Partition_t &partition(const std::string &name, const std::string &from = "") const;
  Partition_t &partition(const std::string &name, const std::string &from = "");
  const Partition_t &partitionWithDupCheck(const std::string &name, bool check = true) const;
  Partition_t &partitionWithDupCheck(const std::string &name, bool check = true);

  uint64_t freeSpaceOf(const std::string &name = "mmcblk0") const; // Get free space of <name> table.

  int hasDuplicateNamedPartition(const std::string &name) const; // Check <name> named partitions are duplicate.

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
  bool
  foreachPartitions(const std::function<bool(const Partition_t &)> &function) const; // For-each input function for partitions (const).
  bool foreachPartitions(const std::function<bool(Partition_t &)> &function); // For-each input function for partitions (non-const).
  bool foreachLogicalPartitions(
      const std::function<bool(const Partition_t &)> &function) const; // For-each input function for logical partitions (const).
  bool foreachLogicalPartitions(
      const std::function<bool(Partition_t &)> &function); // For-each input function for logical partitions (non-const).
  bool foreachGptData(const std::function<bool(const std::filesystem::path &path,
                                               const std::shared_ptr<GPTData> &)> &function)
      const; // For-each input function for gpt data collection (const).
  bool foreachGptData(
      const std::function<bool(const std::filesystem::path &path,
                               std::shared_ptr<GPTData> &)> &function); // For-each input function for gpt data collection (non-const).
  bool foreachFor(const std::vector<std::string> &list,
                  const std::function<bool(const Partition_t &)> &function)
      const; // For-each input function for input partition list (all types, const).
  bool foreachFor(
      const std::vector<std::string> &list,
      const std::function<bool(Partition_t &)> &function); // For-each input function for input partition list (all types, non-const).
  bool foreachForLogicalPartitions(const std::vector<std::string> &list,
                                   const std::function<bool(const Partition_t &)> &function)
      const; // For-each input function for input partition list (only normal partitions, const).
  bool foreachForLogicalPartitions(const std::vector<std::string> &list,
                                   const std::function<bool(Partition_t &)> &function); // For-each input function for input partition
                                                                                        // list (only normal partitions, non-const).
  bool foreachForPartitions(const std::vector<std::string> &list,
                            const std::function<bool(const Partition_t &)> &function)
      const; // For-each input function for input partition list (only normal partitions, const).
  bool foreachForPartitions(const std::vector<std::string> &list,
                            const std::function<bool(Partition_t &)>
                                &function); // For-each input function for input partition list (only normal partitions, non-const).

  void reScan(bool auto_toggled = false);    // Rescan tables. DO NOT USE `reScan(true)` MANUALLY!
  void removeTable(const std::string &name); // Remove table.

  void setTables(std::unordered_set<std::string> names); // Set tables. By names (like sda, sdc, sdg).
  void setGPTDataOf(const std::string &name,
                    std::shared_ptr<GPTData> data); // Set GPTData of <name> table.
  void setAutoScanOnTableChanges(bool state);       // Set auto scanning as <state>.
  void clear();                                     // Cleanup data (excepts auto scan state and seek name).
  void reset();                                     // Cleanup (clear()) and reset variables.

  iterator begin();              // Non-const begin iterator for range-based loop.
  iterator end();                // Non-const end iterator for range-based loop.
  const_iterator begin() const;  // Const begin iterator for range-based loop.
  const_iterator end() const;    // Const end iterator for range-based loop.
  const_iterator cbegin() const; // Const begin iterator for modern C++ range-based loop.
  const_iterator cend() const;   // Const begin iterator for modern C++ range-based loop.

  bool operator==(const Builder &other) const; // pd1 == pd2
  bool operator!=(const Builder &other) const; // pd1 != pd2
  explicit operator bool() const;              // if (pd) { ... } (equals to valid())
  bool operator!() const;                      // if (!pd) { ... } (equals to !valid())

  std::vector<const Partition_t *> operator*() const;                        // std::vector<const Partition_t *> = *pd
  std::vector<Partition_t *> operator*();                                    // std::vector<Partition_t *> = *pd
  const std::shared_ptr<GPTData> &operator[](const std::string &name) const; // const std::shared_ptr<GPTData> data = pd["sda"]
  std::shared_ptr<GPTData> &operator[](const std::string &name);             // std::shared_ptr<GPTData> data = pd["sda"]
  const GPTPart *operator()(const std::string &name, uint32_t index) const;  // const GPTPart* part = pd("sda", 3);
  GPTPart *operator()(const std::string &name, uint32_t index);              // GPTPart* part = pd("sda", 3);

  Builder &operator=(const Builder &other) = default; // pd2 = pd1
  Builder &operator=(Builder &&other) noexcept;       // pd2 = std::move(p1)
};

/*
#if __ANDROID_API__ >= 30 // The current libbase version requires a minimum API level of 30.
using namespace android::fs_mgr;

class SuperManager {
  uint32_t slot;
  std::error_code error;
  std::filesystem::path superPartition;
  std::unique_ptr<LpMetadata> lpMetadata;

  void readMetadata(); // Read metadata from super partition.

public:
  SuperManager();
  explicit SuperManager(const std::filesystem::path &superPartitionPath); // Read metadata from input partition path.
  SuperManager(const SuperManager &other);                                // Copy constructor.
  SuperManager(SuperManager &&other) noexcept;                            // Move constructor.

  static bool isEmptySuperImage(const std::filesystem::path &superImagePath); // Checks ibput super image is empty.

  bool hasPartition(const std::string &name) const;      // Checks <name> partition is existing.
  bool hasFreeSpace() const;                             // Checks has free space in super partition.
  bool hasFreeSpace(const std::string &groupName) const; // Checks has free space in <groupName> named group in super partition.

  std::unique_ptr<LpMetadata> &metadata();         // Get LpMetadata reference (non-const).
  std::unique_ptr<LpMetadata> &metadata() const;   // Get LpMetadata reference (const).
  std::vector<PartitionGroup *> groups();          // Get list of PartitionGroup* as reference list (non-const).
  std::vector<PartitionGroup *> groups() const;    // Get list of PartitionGroup* as reference list (const).
  std::vector<Partition *> &partitions();          // Get list of Partition* as reference list (non-const).
  std::vector<Partition *> &partitions() const;    // Get list of Partitipn* as reference list (const).
  std::vector<std::string> groupNames() const;     // Get list of group names.
  std::vector<std::string> partitionNames() const; // Get list of partition names.

  uint64_t size() const;      // Get size of super partition.
  uint64_t freeSpace() const; // Get free space in super partition.
  uint64_t usedSpace() const; // Get used space in super partition.

  std::error_code error() const;

  bool createPartition(const std::string &name, const std::string &groupName = "",
                       uint32_t attributes = LP_PARTITION_ATTR_NONE);               // Create new partition as <name> named.
  bool deletePartition(const std::string &name, const std::string &groupName = ""); // Delete <name> named partition.
  bool changeGroup(const std::string &name,
                   const std::string &groupName = ""); // Change group of <name> named partition as <groupName> named group.
  bool resizeGroup(const std::string &groupName, uint64_t newSize); // Resize <groupName> named group.
  bool resizePartition(const std::string &name, uint64_t newSize);  // Resize <name> named partititon as <newSize>.
  bool resizePartition(const std::string &name, const std::string &groupName,
                       uint64_t newSize); // Resize <name> named partition in <groupName> named group in as <newSize>.
  bool saveChanges();                     // Write changes to super partition.

  bool dumpSuperImage() const;                            // Dump super image.
  bool dumpPartitionImage(const std::string &name) const; // Dump <name> named partition image.

  void reloadMetadata(); // Reload metadata of super partition.

  SuperManager &operator=(const SuperManager &other);     // Copy operator.
  SuperManager &operator=(SuperManager &&other) noexcept; // Move operator.
}; // class SuperManager

#endif // #if __ANDROID_API__ >= 30
*/

using Error = Helper::Error;

std::string getLibVersion(); // Get version string of library.
std::error_code make_error_code(Partition_t::Errors ec);

namespace Extra {
namespace FileSystemMagic { // Known magics of filesystems.
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

namespace AndroidMagic { // Known magics of android-spefic structures.
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

template <> struct std::is_error_code_enum<PartitionMap::Partition_t::Errors> : true_type {};

// clang-format off
#define FOREACH_PARTITIONS_LAMBDA_PARAMETERS         (PartitionMap::Partition_t & partition)
#define FOREACH_PARTITIONS_LAMBDA_PARAMETERS_CONST   (const PartitionMap::Partition_t & partition)
#define FOREACH_GPT_DATA_LAMBDA_PARAMETERS           (const std::filesystem::path &path, std::shared_ptr<GPTData> &gptData)
#define FOREACH_GPT_DATA_LAMBDA_PARAMETERS_CONST     (const std::filesystem::path &path, const std::shared_ptr<GPTData> &gptData)
// clang-format on

#endif // #ifndef LIBPARTITION_MAP_LIB_HPP
