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
#include <libhelper/definations.hpp>
#include <libpartition_map/partition.hpp>

namespace PartitionMap {

class Builder {
  std::map<std::filesystem::path, std::shared_ptr<GPTData>> gptDataCollection;
  std::vector<Partition_t> localPartitions;
  std::unordered_set<std::string> localTableNames;

  bool buildAutoOnDiskChanges, isUFS;

  void scan();
  void scanLogicalPartitions();
  void findTablePaths();
  void reScan(bool auto_toggled);

public:
  using list_t = std::vector<std::reference_wrapper<Partition_t>>;
  using const_list_t = std::vector<std::reference_wrapper<const Partition_t>>;
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
      : gptDataCollection(std::move(other.gptDataCollection)), localPartitions(std::move(other.localPartitions)),
        localTableNames(std::move(other.localTableNames)), buildAutoOnDiskChanges(other.buildAutoOnDiskChanges),
        isUFS(other.isUFS) { // Builder map(std::move(otherMap))
    other.buildAutoOnDiskChanges = true;
    other.isUFS = false;
  }

  list_t allPartitions();                            // Get references of all partitions (std::vector<Partition_t*>, non-const).
  const_list_t allPartitions() const;                // Get references of all partitions (std::vector<Partition_t*>, const).
  list_t partitions();                               // Get references of partitions (std::vector<Partition_t*>, non-const).
  const_list_t partitions() const;                   // Get references of partitions (std::vector<const Partition_t*>, const).
  list_t logicalPartitions();                        // Get references of logical partitions (non-const).
  const_list_t logicalPartitions() const;            // Get references of logical partitions (const).
  list_t partitionsByTable(const std::string &name); // Get references of partitions of table by name (non-const).
  const_list_t partitionsByTable(const std::string &name) const; // Get references of partitions of table by name (const).

  std::vector<std::pair<bool, std::string>> duplicatePartitionPositions(const std::string &name) const;

  const std::unordered_set<std::string> &tableNames() const;    // Get references of table names (const)
  std::unordered_set<std::string> &tableNames();                // Get references of table names (non-const).
  std::unordered_set<std::filesystem::path> tablePaths() const; // Get table paths (form gptDataCollection).

  const std::map<std::filesystem::path, std::shared_ptr<GPTData>> &allGPTData() const; // Get gptDataCollection.

  const std::shared_ptr<GPTData> &GPTDataOf(
      const std::string &name) const; // Get gpt data of table by name (retrieve it as const std::shared_ptr as it is stored, const).
  std::shared_ptr<GPTData> &
  GPTDataOf(const std::string &name); // Get gpt data of table by name (retrieve it as std::shared_ptr as it is stored, non-const).

  std::vector<std::pair<std::string, uint64_t>> dataOfLogicalPartitions() const; // Get logical partition information.
  std::vector<BasicInfo> dataOfPartitions() const;                               // Get information about partitions.
  std::vector<BasicInfo> dataOfPartitionsByTable(const std::string &name) const; // Get information about partitions by table name.

  std::optional<std::reference_wrapper<const Partition_t>> partition(const std::string &name, const std::string &from = "") const;
  std::optional<std::reference_wrapper<Partition_t>> partition(const std::string &name, const std::string &from = "");
  std::optional<std::reference_wrapper<const Partition_t>> partitionWithDupCheck(const std::string &name, bool check = true) const;
  std::optional<std::reference_wrapper<Partition_t>> partitionWithDupCheck(const std::string &name, bool check = true);

  uint64_t freeSpaceOf(const std::string &name) const; // Get free space of <name> table.

  int hasDuplicateNamedPartition(const std::string &name) const; // Check <name> named partitions are duplicate.

  bool hasPartition(const std::string &name) const;        // Check <name> partition is existing or not (checks all).
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

  // For-each input function for all partitions (const).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEach(F &&function) const {
    LOGI << "Foreaching input function for all partitions." << std::endl;
    bool isSuccess = true;
    for (auto &part : localPartitions)
      isSuccess &= function(part);

    return isSuccess;
  }

  // For-each input function for all partitions (non-const).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEach(F &&function) {
    LOGI << "Foreaching input function for all partitions." << std::endl;
    bool isSuccess = true;
    for (auto &part : localPartitions)
      isSuccess &= function(part);

    return isSuccess;
  }

  // For-each input function for partitions (const).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEachPartitions(F &&function) const {
    LOGI << "Foreaching input function for normal partitions." << std::endl;
    bool isSuccess = true;
    for (auto &part : partitions())
      isSuccess &= function(part);

    return isSuccess;
  }

  // For-each input function for partitions (non-const).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEachPartitions(F &&function) {
    LOGI << "Foreaching input function for normal partitions." << std::endl;
    bool isSuccess = true;
    for (auto &part : partitions())
      isSuccess &= function(part);

    return isSuccess;
  }

  // For-each input function for logical partitions (const).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEachLogicalPartitions(F &&function) const {
    LOGI << "Foreaching input function for logical partitions." << std::endl;
    bool isSuccess = true;
    for (auto &part : logicalPartitions())
      isSuccess &= function(part);

    return isSuccess;
  }

  // For-each input function for logical partitions (non-const).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEachLogicalPartitions(F &&function) {
    LOGI << "Foreaching input function for logical partitions." << std::endl;
    bool isSuccess = true;
    for (auto &part : logicalPartitions())
      isSuccess &= function(part);

    return isSuccess;
  }

  // For-each input function for gpt data collection (const).
  template <typename F>
    requires Helper::Invocable<F, bool, const std::filesystem::path &, const std::shared_ptr<GPTData> &>
  bool forEachGptData(F &&function) const {
    LOGI << "Foreaching input function for all GPTData data." << std::endl;
    bool isSuccess = true;
    for (auto &[path, gptData] : gptDataCollection)
      isSuccess &= function(path, gptData);

    return isSuccess;
  }

  // For-each input function for gpt data collection (non-const).
  template <typename F>
    requires Helper::Invocable<F, bool, const std::filesystem::path &, std::shared_ptr<GPTData> &>
  bool forEachGptData(F &&function) {
    LOGI << "Foreaching input function for all GPTData data." << std::endl;
    bool isSuccess = true;
    for (auto &[path, gptData] : gptDataCollection)
      isSuccess &= function(path, gptData);

    return isSuccess;
  }

  // For-each input function for input partition list (all types, const).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEachFor(const std::vector<std::string> &list, F &&function) const {
    LOGI << "Foreaching input function for input list." << std::endl;
    bool isSuccess = true;
    for (auto &name : list) {
      if (hasPartition(name) || hasLogicalPartition(name)) isSuccess &= function(partition(name)->get());
    }

    return isSuccess;
  }

  // For-each input function for input partition list (all types, non-const).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEachFor(const std::vector<std::string> &list, F &&function) {
    LOGI << "Foreaching input function for input list." << std::endl;
    bool isSuccess = true;
    for (auto &name : list) {
      if (hasPartition(name) || hasLogicalPartition(name)) isSuccess &= function(partition(name)->get());
    }

    return isSuccess;
  }

  // For-each input function for input partition list (only logical partitions, const).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEachForLogicalPartitions(const std::vector<std::string> &list, F &&function) const {
    LOGI << "Foreaching input function for input list (only for logical partitions)." << std::endl;
    bool isSuccess = true;
    for (auto &name : list) {
      if (hasLogicalPartition(name)) isSuccess &= function(partition(name));
    }

    return isSuccess;
  }

  // For-each input function for input partition list (only logical partitions, non-const).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEachForLogicalPartitions(const std::vector<std::string> &list, F &&function) {
    LOGI << "Foreaching input function for input list (only for logical partitions)." << std::endl;
    bool isSuccess = true;
    for (auto &name : list) {
      if (hasLogicalPartition(name)) isSuccess &= function(partition(name));
    }

    return isSuccess;
  }

  // For-each input function for input partition list (only normal partitions, const).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEachPartitions(const std::vector<std::string> &list, F &&function) const {
    LOGI << "Foreaching input function for input list (only normal partitions)." << std::endl;
    bool isSuccess = true;
    for (auto &name : list) {
      if (hasPartition(name)) isSuccess &= function(partition(name));
    }

    return isSuccess;
  }

  // For-each input function for input partition list (only normal partitions, non-const).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEachPartitions(const std::vector<std::string> &list, F &&function) {
    LOGI << "Foreaching input function for input list (only normal partitions)." << std::endl;
    bool isSuccess = true;
    for (auto &name : list) {
      if (hasPartition(name)) isSuccess &= function(partition(name));
    }

    return isSuccess;
  }

  void reScan();                             // Rescan tables.
  void removeTable(const std::string &name); // Remove table.

  void setTables(const std::unordered_set<std::string> &tables);      // Set tables. By names (like sda, sdc, sdg).
  Builder &withTables(const std::unordered_set<std::string> &tables); // Set tables (chain). By names (like sda, sdc, sdg).

  void setAutoScan(bool state);      // Set auto scanning as <state>.
  Builder &withAutoScan(bool state); // Set auto scanning as <state>.

  void setGPTDataOf(const std::string &name,
                    std::shared_ptr<GPTData> data); // Set GPTData of <name> table.

  void clear(); // Cleanup data (excepts auto scan state and seek name).
  void reset(); // Cleanup (clear()) and reset variables.

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

  const_list_t operator*() const;                                            // std::vector<const Partition_t *> = *pd
  list_t operator*();                                                        // std::vector<Partition_t *> = *pd
  const std::shared_ptr<GPTData> &operator[](const std::string &name) const; // const std::shared_ptr<GPTData> data = pd["sda"]
  std::shared_ptr<GPTData> &operator[](const std::string &name);             // std::shared_ptr<GPTData> data = pd["sda"]
  const GPTPart *operator()(const std::string &name, uint32_t index) const;  // const GPTPart* part = pd("sda", 3);
  GPTPart *operator()(const std::string &name, uint32_t index);              // GPTPart* part = pd("sda", 3);

  Builder &operator=(const Builder &other) = default; // pd2 = pd1
  Builder &operator=(Builder &&other) noexcept;       // pd2 = std::move(p1)
}; // class Builder

} // namespace PartitionMap

#endif // #ifndef LIBPARTITION_MAP_BUILDER_HPP
