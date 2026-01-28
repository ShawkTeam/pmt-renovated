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

#ifndef LIBPARTITION_MAP_LIB_HPP
#define LIBPARTITION_MAP_LIB_HPP

#include <filesystem>
#include <functional>
#include <gpt.h>
#include <libhelper/lib.hpp>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace PartitionMap {

enum SizeUnit : int { BYTE = 1, KiB = 2, MiB = 3, GiB = 4 };

class Partition_t {
private:
  std::filesystem::path diskPath; // The disk path to which the partition
                                  // belongs (like /dev/block/sdc).
  uint32_t index = 0;             // The actual index of the partition within the table.
  GPTPart gptPart;                // Complete data for the partition.

public:
  struct BasicData {
    GPTPart gptPart;
    uint32_t index;
    std::filesystem::path diskPath;
  };

  Partition_t() : gptPart(GPTPart()) {}
  Partition_t(const Partition_t &other) = default;
  explicit Partition_t(const BasicData &input) : diskPath(input.diskPath), index(input.index), gptPart(input.gptPart) {}

  GPTPart getGPTPart();     // Get copy of GPTPart data.
  GPTPart *getGPTPartRef(); // Get reference of GPTPart data.

  std::filesystem::path getPath() const;     // Get partition path (like /dev/block/sdc4)
  std::filesystem::path getDiskPath() const; // Get diskPath variable.
  std::filesystem::path getPathByName();     // Get partition path by name.

  std::string getName();           // Get partition name.
  std::string getDiskName() const; // Get disk name.
  std::string getFormattedSizeString(SizeUnit size_unit,
                                     bool no_type = false) const; // Get partition size as formatted string.

  std::string getGUIDAsString() const; // Get partition GUID as string.

  uint32_t getIndex() const;                               // Get partition index in GPT table.
  uint64_t getSize(uint32_t sectorSize = 4096) const;      // Get partition size in bytes.
  uint64_t getStartByte(uint32_t sectorSize = 4096) const; // Starting byte address.
  uint64_t getEndByte(uint32_t sectorSize = 4096) const;   // Ending byte address.

  GUIDData getGUID() const; // Get partition GUID.

  void set(const BasicData &data);              // Set GPTPart object, index and disk path.
  void setIndex(uint32_t new_index);            // Set partition index.
  void setDiskPath(const std::string &path);    // Set disk path.
  void setGptPart(const GPTPart &otherGptPart); // Sset GPTPart object.

  bool isSuperPartition() const; // Checks whether the partition is dynamic or not.
  bool empty();                  // Checks whether the partition info is empty or not.

  bool operator==(const Partition_t &other) const; // p1 == p2
  bool operator==(const GUIDData &other) const;    // p1 == guid
  bool operator!=(const Partition_t &other) const; // p1 != p2
  bool operator!=(const GUIDData &other) const;    // p1 != guid
  explicit operator bool();                        // p (equals !empty())
  bool operator!();                                // !p (equals empty())

  Partition_t &operator=(const Partition_t &other) = default; // p1 = p2
  friend std::ostream &operator<<(std::ostream &os,
                                  Partition_t &other); // std::cout << p1
}; // class Partition_t

class LogicalPartition_t {
private:
  std::filesystem::path partitionPath;

public:
  class Extra {
  public:
    static bool isReallyLogical(const std::filesystem::path &path);
  };

  LogicalPartition_t() = default;

  LogicalPartition_t(const LogicalPartition_t &other) : partitionPath(other.partitionPath) {
    if (!Extra::isReallyLogical(partitionPath))
      throw std::invalid_argument("Invalid partition path! It's not logical partition!");
  }
  explicit LogicalPartition_t(std::filesystem::path path) : partitionPath(std::move(path)) {
    if (!Extra::isReallyLogical(partitionPath))
      throw std::invalid_argument("Invalid partition path! It's not logical partition!");
  }

  std::filesystem::path getPath() const;         // Get partition path (like /dev/block/mapper/system).
  std::filesystem::path getAbsolutePath() const; // Get partition path (like /dev/block/dm-3).

  std::string getName() const; // Get partition name.

  uint64_t getSize() const; // Get partition size in bytes.

  bool empty() const; // Checks whether the partition info is empty or not.

  void setPartitionPath(const std::filesystem::path &path); // Set partition path.

  bool operator==(const LogicalPartition_t &other) const; // lp1 == lp2
  bool operator!=(const LogicalPartition_t &other) const; // lp1 != lp2
  explicit operator bool() const;                         // lp (equals to !empty())
  bool operator!() const;                                 // !lp (equlas to empty())

  LogicalPartition_t &operator=(const LogicalPartition_t &other) = default; // lp1 = lp2
};

class Builder {
private:
  std::vector<Partition_t> partitions;
  std::vector<LogicalPartition_t> logicalPartitions;
  std::map<std::filesystem::path, std::shared_ptr<GPTData>> gptDataCollection;
  std::unordered_set<std::string> diskNames;

  bool buildAutoOnDiskChanges, isUFS;
  std::string seek;

  void scan();
  void scanLogicalPartitions();
  void findDiskPaths();

public:
  Builder() : buildAutoOnDiskChanges(true), isUFS(false) {
    findDiskPaths();
    scan();
    scanLogicalPartitions();
  }

  std::vector<Partition_t> getPartitions() const; // Get partitions (std::vector<Partition_t> partitions).
  std::vector<Partition_t> getPartitionsByDisk(const std::string &name) const; // Get partitions of disk by name.
  std::vector<LogicalPartition_t> getLogicalPartitions() const; // Get logical partitions (logicalPartitions).

  std::unordered_set<std::string> getDiskNames() const;           // Get disk names (diskNames).
  std::unordered_set<std::filesystem::path> getDiskPaths() const; // Get disk paths (form gptDataCollection).

  const std::map<std::filesystem::path, std::shared_ptr<GPTData>> &getAllGPTData() const; // Get gptDataCollection.

  const std::shared_ptr<GPTData> &getGPTDataOf(const std::string &name) const; // Get gpt data of disk by name.

  std::vector<std::pair<std::string, uint64_t>>
  getDataOfLogicalPartitions() const; // Get logical partition information.

  std::vector<std::tuple<std::string, uint64_t, bool>> getDataOfPartitions(); // Get information about partitions.
  std::vector<std::tuple<std::string, uint64_t, bool>>
  getDataOfPartitionsByDisk(const std::string &name); // Get information about partitions by disk name.

  std::string getSeek() const; // Get the disk name that the [uint32_t] operator will use.

  bool hasPartition(const std::string &name);              // Check <name> partition is existing or not.
  bool hasLogicalPartition(const std::string &name) const; // Check <name> logical partition is existing or not.
  bool hasDisk(const std::string &name) const;             // Check <name> disk name is existing or not.
  bool isUsesUFS() const;                                  // Get the device uses UFS or not.
  bool isHasSuperPartition() const;                        // Get any disk has super partition or not.
  bool isLogical(const std::string &name) const;           // Check <name> is logical partition or
                                                           // not. Same as hasLogicalPartition().
  bool empty() const;                                      // Checks partitions, logicalPartitions and
                                                           // gptDataCollection is empty or not.
  bool diskNamesEmpty() const;                             // Checks diskNames is empty or not.
  bool valid();                                            // Validate GPTData collection integrity. Checks with
                                                           // GPTData::Verify() and GPTData::CheckHeaderValidity().
  bool
  foreachPartitions(const std::function<bool(Partition_t &)> &function); // For-each input function for partition list.
  bool foreachLogicalPartitions(
      const std::function<bool(LogicalPartition_t &)> &function); // For-each input function for logical partition list.
  bool foreachGptData(const std::function<bool(const std::filesystem::path &,
                                               std::shared_ptr<GPTData> &)>
                          &function); // For-each input function for gpt data collection.

  void reScan(bool auto_toggle = false);    // Rescan disks.
  void addDisk(const std::string &name);    // Add disk.
  void removeDisk(const std::string &name); // Remove disk.
  void setSeek(const std::string &name);    // Set the disk name that the
                                            // [uint32_t] operator will use.

  void setDisks(std::unordered_set<std::string> names); // Set disks. By names (like sda, sdc, sdg).
  void setGPTDataOf(const std::string &name,
                    std::shared_ptr<GPTData> data); // Set GPTData of <name> disk.
  void setAutoScanOnDiskChanges(bool state);        // Set auto scanning as <state>.
  void clear();                                     // Cleanup data (excepts auto scan state and seek name).
  void reset();                                     // Cleanup (clear()) and reset variables.

  class Extra {
  public:
    static bool isReallyDisk(const std::string &name);
  };

  bool operator==(const Builder &other); // pd1 == pd2
  bool operator!=(const Builder &other); // pd1 != pd2
  explicit operator bool();              // if (pd) { ... } (equals to valid())
  bool operator!();                      // if (!pd) { ... } (equals to !valid())

  const std::shared_ptr<GPTData> &
  operator[](const std::string &name) const; // std::shared_ptr<GPTData> data = pd["sda"]
  GPTPart operator[](uint32_t index);        // GPTPart part = pd[2]

  Builder &operator=(const Builder &other) = default; // pd2 = pd1
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
} // namespace Extra
} // namespace PartitionMap

#define FOREACH_PARTITIONS_LAMBDA_PARAMETERS (PartitionMap::Partition_t & partition)
#define FOREACH_LOGICAL_PARTITIONS_LAMBDA_PARAMETERS (PartitionMap::LogicalPartition_t & lpartition)
#define FOREACH_GPT_DATA_LAMBDA_PARAMETERS (const std::filesystem::path &path, std::shared_ptr<GPTData> &gptData)

#endif // #ifndef LIBPARTITION_MAP_LIB_HPP
