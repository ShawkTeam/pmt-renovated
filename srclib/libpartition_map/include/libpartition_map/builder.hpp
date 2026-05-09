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
 * @file builder.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief The header of the @c libpartition_map builder class.
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

/**
 * @namespace PartitionMap
 * @brief Main namespace of libpartition_map library.
 */
namespace PartitionMap {

/**
 * @brief A class that examines and neatly lists all storage units on the device. It records information about the partitions, etc.
 * @note This class collects data using the Partition_t class.
 * @see PartitionMap::Partition_t
 * @see [GPT fdisk](https://android.googlesource.com/platform/external/gptfdisk)
 */
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
  /// @brief List type.
  using list_t = std::vector<std::reference_wrapper<Partition_t>>;
  /// @brief Constant list type.
  using const_list_t = std::vector<std::reference_wrapper<const Partition_t>>;

  /// @brief Iterator.
  using iterator = std::vector<Partition_t>::iterator;
  /// @brief Constant iterator.
  using const_iterator = std::vector<Partition_t>::const_iterator;

  /// @note This class cannot be constructible; its purpose is to function like a namespace.
  class Extra {
  public:
    Extra() = delete;

    /// @brief Verify that the entry provided is a partition table.
    static bool isReallyTable(const std::string &name);
  };

  /// @brief Default constructor, initializes variables, etc.
  Builder() : buildAutoOnDiskChanges(true), isUFS(false) {
    findTablePaths();
    scan();
    scanLogicalPartitions();
  }

  /// @brief Copy constructor.
  Builder(const Builder &other) = default;

  /// @brief Move constructor.
  Builder(Builder &&other) noexcept
      : gptDataCollection(std::move(other.gptDataCollection)), localPartitions(std::move(other.localPartitions)),
        localTableNames(std::move(other.localTableNames)), buildAutoOnDiskChanges(other.buildAutoOnDiskChanges), isUFS(other.isUFS) {
    other.buildAutoOnDiskChanges = true;
    other.isUFS = false;
  }

  /// @brief Get references of all partitions.
  list_t allPartitions();
  /// @brief Get references of all partitions.
  const_list_t allPartitions() const;
  /// @brief Get references of partitions.
  list_t partitions();
  /// @brief Get references of partitions.
  const_list_t partitions() const;
  /// @brief Get references of logical partitions.
  list_t logicalPartitions();
  /// @brief Get references of logical partitions.
  const_list_t logicalPartitions() const;
  /// @brief Get references of partitions of table by name.
  list_t partitionsByTable(const std::string &name);
  /// @brief Get references of partitions of table by name.
  const_list_t partitionsByTable(const std::string &name) const;

  /**
   * @brief Find the partition with the same names.
   * @param name Partition name.
   * @note The first parameter of the pair actively provides used partition. For actively used partition, the value is @c true.
   *       The second parameter holds the name of the table where the partition is located. If the partition being searched is logical,
   * it is always empty.
   */
  std::vector<std::pair<bool, std::string>> duplicatePartitionPositions(const std::string &name) const;

  /// @brief Get references of table names (constant).
  const std::unordered_set<std::string> &tableNames() const;
  /// @brief Get references of table names (non-constant).
  std::unordered_set<std::string> &tableNames();
  /// @brief Get table paths (from @c gptDataCollection ).
  std::unordered_set<std::filesystem::path> tablePaths() const;

  /**
   * @brief Get @c gptDataCollection.
   * @note The first parameter holds the path to the partition table.
   *       The second parameter holds GPTData as a std::shared_ptr.
   */
  const std::map<std::filesystem::path, std::shared_ptr<GPTData>> &allGPTData() const;

  /**
   * @brief Get gpt data of table by name.
   * @param name Table name.
   * @return <tt>const std::shared_ptr</tt> as it is stored.
   */
  const std::shared_ptr<GPTData> &GPTDataOf(const std::string &name) const;

  /**
   * @brief Get gpt data of table by name.
   * @param name Table name.
   * @return @c std::shared_ptr as it is stored.
   */
  std::shared_ptr<GPTData> &GPTDataOf(const std::string &name);

  /**
   * @brief Get logical partition informations.
   * @note The first parameter of the pair holds the name of the partition.
   *       The second parameter holds the size of the partition.
   */
  std::vector<std::pair<std::string, uint64_t>> dataOfLogicalPartitions() const;

  /// @brief Get information about partitions.
  std::vector<BasicInfo> dataOfPartitions() const;
  /// @brief Get information about partitions by table name.
  std::vector<BasicInfo> dataOfPartitionsByTable(const std::string &name) const;

  /**
   * @brief Get Partition_t object of needed partition.
   * @param name Partition name.
   * @param from Table name to search for the partition.
   * @retval std::nullopt Partition not found.
   * @retval "std::reference_wrapper<const Partition_t>" Partition is found.
   */
  std::optional<std::reference_wrapper<const Partition_t>> partition(const std::string &name, const std::string &from = "") const;

  /**
   * @brief Get Partition_t object of needed partition.
   * @param name Partition name.
   * @param from Table name to search for the partition.
   * @retval std::nullopt Partition not found.
   * @retval std::reference_wrapper<Partition_t> Partition is found.
   */
  std::optional<std::reference_wrapper<Partition_t>> partition(const std::string &name, const std::string &from = "");

  /**
   * @brief Get Partition_t object of needed partition.
   * @param name Partition name.
   * @param check Before retrieving the partition object, perform a duplicate check using @c duplicatePartitionPositions.
   *              Default is true.
   * @retval std::nullopt Partition not found.
   * @retval "std::reference_wrapper<const Partition_t>" Partition is found.
   */
  std::optional<std::reference_wrapper<const Partition_t>> partitionWithDupCheck(const std::string &name, bool check = true) const;

  /**
   * @brief Get Partition_t object of needed partition.
   * @param name Partition name.
   * @param check Before retrieving the partition object, perform a duplicate check using @c duplicatePartitionPositions. Default is
   * false.
   * @retval std::nullopt Partition not found.
   * @retval std::reference_wrapper<Partition_t> Partition is found.
   */
  std::optional<std::reference_wrapper<Partition_t>> partitionWithDupCheck(const std::string &name, bool check = true);

  /// @brief Get free space of the partition table.
  uint64_t freeSpaceOf(const std::string &name) const;

  /// @brief Check if there are any duplicate partitions.
  int hasDuplicateNamedPartition(const std::string &name) const;

  /// @brief Check the availability of the partition.
  bool hasPartition(const std::string &name) const;
  /// @brief Check whether the partition is logical.
  bool hasLogicalPartition(const std::string &name) const;
  /// @brief Check whether the partition table is available.
  bool hasTable(const std::string &name) const;
  /// @brief Check the device is uses UFS.
  bool isUsesUFS() const;
  /// @brief Check whether the super partition exists in any partition table.
  bool isHasSuperPartition() const;
  /// @brief Check whether the partition is logical. Same as @c hasLogicalPartition().
  bool isLogical(const std::string &name) const;

  /// @brief Checks @c partitions, @c logicalPartitions and @c gptDataCollection is empty.
  bool empty() const;

  /// @brief Checks @c tableNames is empty.
  bool tableNamesEmpty() const;

  /// @brief Validate GPTData collection integrity. Checks with @c GPTData::Verify() and @c GPTData::CheckHeaderValidity().
  bool valid() const;

  /// @brief For-each input function for all partitions (constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEach(F &&function) const {
    LOGI << "Foreaching input function for all partitions." << std::endl;
    bool isSuccess = true;
    for (auto &part : localPartitions)
      isSuccess &= function(part);

    return isSuccess;
  }

  /// @brief For-each input function for all partitions (non-constant).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEach(F &&function) {
    LOGI << "Foreaching input function for all partitions." << std::endl;
    bool isSuccess = true;
    for (auto &part : localPartitions)
      isSuccess &= function(part);

    return isSuccess;
  }

  /// @brief For-each input function for partitions (constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEachPartitions(F &&function) const {
    LOGI << "Foreaching input function for normal partitions." << std::endl;
    bool isSuccess = true;
    for (auto &part : partitions())
      isSuccess &= function(part);

    return isSuccess;
  }

  /// For-each input function for partitions (non-constant).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEachPartitions(F &&function) {
    LOGI << "Foreaching input function for normal partitions." << std::endl;
    bool isSuccess = true;
    for (auto &part : partitions())
      isSuccess &= function(part);

    return isSuccess;
  }

  /// @brief For-each input function for logical partitions (constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEachLogicalPartitions(F &&function) const {
    LOGI << "Foreaching input function for logical partitions." << std::endl;
    bool isSuccess = true;
    for (auto &part : logicalPartitions())
      isSuccess &= function(part);

    return isSuccess;
  }

  /// @brief For-each input function for logical partitions (non-constant).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEachLogicalPartitions(F &&function) {
    LOGI << "Foreaching input function for logical partitions." << std::endl;
    bool isSuccess = true;
    for (auto &part : logicalPartitions())
      isSuccess &= function(part);

    return isSuccess;
  }

  /// @brief For-each input function for gpt data collection (constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const std::filesystem::path &, const std::shared_ptr<GPTData> &>
  bool forEachGptData(F &&function) const {
    LOGI << "Foreaching input function for all GPTData data." << std::endl;
    bool isSuccess = true;
    for (auto &[path, gptData] : gptDataCollection)
      isSuccess &= function(path, gptData);

    return isSuccess;
  }

  /// @brief For-each input function for gpt data collection (non-constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const std::filesystem::path &, std::shared_ptr<GPTData> &>
  bool forEachGptData(F &&function) {
    LOGI << "Foreaching input function for all GPTData data." << std::endl;
    bool isSuccess = true;
    for (auto &[path, gptData] : gptDataCollection)
      isSuccess &= function(path, gptData);

    return isSuccess;
  }

  /// @brief For-each input function for input partition list (all types, constant).
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

  /// @brief For-each input function for input partition list (all types, non-constant).
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

  /// @brief For-each input function for input partition list (only logical partitions, constant).
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

  /// @brief For-each input function for input partition list (only logical partitions, non-constant).
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

  /// @brief For-each input function for input partition list (only normal partitions, constant).
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

  /// @brief For-each input function for input partition list (only normal partitions, non-constant).
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

  /// @brief Rescan tables.
  void reScan();
  /// @brief Removes table.
  void removeTable(const std::string &name);

  /// @brief Set tables by names (like @c sda, @c sdc, @c sdg ).
  void setTables(const std::unordered_set<std::string> &tables);

  /// @brief Set tables (chain function) by names (like @c sda, @c sdc, @c sdg).
  Builder &withTables(const std::unordered_set<std::string> &tables);

  /// @brief Change auto scanning state.
  void setAutoScan(bool state);
  /// @brief Change auto scanning state (chain function).
  Builder &withAutoScan(bool state);

  /// @brief Set GPTData of the table.
  void setGPTDataOf(const std::string &name, std::shared_ptr<GPTData> data);

  /// @brief Cleanup data (excepts auto scan state and seek name).
  void clear();
  /// @brief Cleanup (<tt>clear()</tt>) and reset variables.
  void reset();

  /**
   * @name Builder's iterators and operators.
   * @brief Iterator and operator functions of @c Builder.
   * @{
   */
  iterator begin();              ///< Non-const begin iterator for range-based loop.
  iterator end();                ///< Non-const end iterator for range-based loop.
  const_iterator begin() const;  ///< Const begin iterator for range-based loop.
  const_iterator end() const;    ///< Const end iterator for range-based loop.
  const_iterator cbegin() const; ///< Const begin iterator for modern C++ range-based loop.
  const_iterator cend() const;   ///< Const begin iterator for modern C++ range-based loop.

  bool operator==(const Builder &other) const; ///< @c == assignment.
  bool operator!=(const Builder &other) const; ///< @c != assignment.
  explicit operator bool() const;              ///< Same as @c valid().
  bool operator!() const;                      ///< Same as @c valid().

  const_list_t operator*() const; ///< <tt>std::vector<const Partition_t *> = *pd;</tt>
  list_t operator*();             ///< <tt>std::vector<Partition_t *> = *pd;</tt>
  const std::shared_ptr<GPTData> &
  operator[](const std::string &name) const;                     ///< <tt>const std::shared_ptr<GPTData> data = pd["sda"];</tt>
  std::shared_ptr<GPTData> &operator[](const std::string &name); ///< <tt>std::shared_ptr<GPTData> data = pd["sda"];</tt>
  const GPTPart *operator()(const std::string &name, uint32_t index) const; ///< <tt>const GPTPart* part = pd("sda", 3);</tt>
  GPTPart *operator()(const std::string &name, uint32_t index);             ///< <tt>GPTPart* part = pd("sda", 3);</tt>F

  Builder &operator=(const Builder &other) = default; ///< Copier @c = operator.
  Builder &operator=(Builder &&other) noexcept;       ///< Mover @c = operator.

  /** @} */
}; // class Builder

} // namespace PartitionMap

#endif // #ifndef LIBPARTITION_MAP_BUILDER_HPP
