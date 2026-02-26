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

#ifndef LIBPARTITION_MAP_BUILDER_HPP
#define LIBPARTITION_MAP_BUILDER_HPP

#include <vector>
#include <filesystem>
#include <map>
#include <unordered_set>
#include <gpt.h>
#include <libpartition_map/partition.hpp>

namespace PartitionMap {

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

} // namespace PartitionMap

#endif // #ifndef LIBPARTITION_MAP_BUILDER_HPP
