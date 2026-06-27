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
 * @file table_data_collection.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief @c libpartition_map table data collection.
 */

#ifndef LIBPARTITION_MAP_TABLE_DATA_COLLECTION_HPP
#define LIBPARTITION_MAP_TABLE_DATA_COLLECTION_HPP

#include <vector>
#include <filesystem>
#include <map>
#include <unordered_set>
#include <cassert>
#include <gpt.h>
#include <libhelper/definations.hpp>
#include <libpartition_map/partition.hpp>
#include <liblp/liblp.h>
#include <liblp/metadata_format.h>

namespace PartitionMap {

/// @brief Base table data class.
class BaseTableData {
public:
  using list_t = std::vector<std::reference_wrapper<Partition_t>>;
  using const_list_t = std::vector<std::reference_wrapper<const Partition_t>>;
  using iterator = std::vector<Partition_t>::iterator;
  using const_iterator = std::vector<Partition_t>::const_iterator;

  BaseTableData() = default;
  BaseTableData(const BaseTableData &) = default;
  BaseTableData(BaseTableData &&) noexcept = default;

  virtual constexpr TableType type() const noexcept = 0;

  virtual list_t partitions() = 0;
  virtual const_list_t partitions() const = 0;

  virtual std::vector<BasicInfo> aboutPartitions() const = 0;

  virtual std::optional<std::reference_wrapper<const Partition_t>> partition(const std::string &, const std::string & = "") const = 0;
  virtual std::optional<std::reference_wrapper<Partition_t>> partition(const std::string &, const std::string & = "") = 0;

  virtual bool hasPartition(const std::string &) const = 0;
  virtual bool isSupported() const noexcept = 0;
  virtual constexpr bool isLogical(const std::string &) const = 0;

  virtual bool empty() const = 0;
  virtual bool valid() const = 0;
  virtual void clear() = 0;
  virtual void reset() = 0;
  virtual void reScan() = 0;

  virtual iterator begin() = 0;
  virtual iterator end() = 0;
  virtual const_iterator begin() const = 0;
  virtual const_iterator end() const = 0;
  virtual const_iterator cbegin() const = 0;
  virtual const_iterator cend() const = 0;

  bool operator==(const BaseTableData &) const = default;
  bool operator!=(const BaseTableData &) const = default;
  virtual explicit operator bool() const = 0;
  virtual bool operator!() const = 0;

  BaseTableData &operator=(const BaseTableData &) = default;
  BaseTableData &operator=(BaseTableData &&) noexcept = default;
}; // class BaseTableData

/**
 * @brief A class that examines and neatly lists all storage units on the device. It records information about the partitions, etc.
 * @note This class collects data using the Partition_t class.
 * @see PartitionMap::Partition_t
 * @see [GPT fdisk](https://android.googlesource.com/platform/external/gptfdisk)
 */
class PartitionTableData : public BaseTableData {
  std::map<std::filesystem::path, std::shared_ptr<GPTData>> gptDataCollection;
  std::vector<Partition_t> localPartitions;
  std::unordered_set<std::string> localTableNames;

  bool buildAutoOnDiskChanges, isUFS;

  void scan();
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
  PartitionTableData() : buildAutoOnDiskChanges(true), isUFS(false) {
    findTablePaths();
    scan();
  }

  /// @brief Copy constructor.
  PartitionTableData(const PartitionTableData &other) = default;

  /// @brief Move constructor.
  PartitionTableData(PartitionTableData &&other) noexcept
      : gptDataCollection(std::move(other.gptDataCollection)), localPartitions(std::move(other.localPartitions)),
        localTableNames(std::move(other.localTableNames)), buildAutoOnDiskChanges(other.buildAutoOnDiskChanges), isUFS(other.isUFS) {
    other.buildAutoOnDiskChanges = true;
    other.isUFS = false;
  }

  constexpr TableType type() const noexcept override { return TableType::CLASSIC; }

  static PartitionTableData *cast(BaseTableData *base) {
    assert(dynamic_cast<PartitionTableData *>(base) != nullptr);
    return static_cast<PartitionTableData *>(base);
  }

  static const PartitionTableData *cast(const BaseTableData *base) {
    assert(dynamic_cast<const PartitionTableData *>(base) != nullptr);
    return static_cast<const PartitionTableData *>(base);
  }

  /// @brief Get references of partitions.
  list_t partitions() override;
  /// @brief Get references of partitions.
  const_list_t partitions() const override;
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

  /// @brief Get information about partitions.
  std::vector<BasicInfo> aboutPartitions() const override;
  /// @brief Get information about partitions by table name.
  std::vector<BasicInfo> aboutPartitionsByTable(const std::string &name) const;

  /**
   * @brief Get Partition_t object of needed partition.
   * @param name Partition name.
   * @param from Table name to search for the partition.
   * @retval std::nullopt Partition not found.
   * @retval "std::reference_wrapper<const Partition_t>" Partition is found.
   */
  std::optional<std::reference_wrapper<const Partition_t>> partition(const std::string &name,
                                                                     const std::string &from = "") const override;

  /**
   * @brief Get Partition_t object of needed partition.
   * @param name Partition name.
   * @param from Table name to search for the partition.
   * @retval std::nullopt Partition not found.
   * @retval std::reference_wrapper<Partition_t> Partition is found.
   */
  std::optional<std::reference_wrapper<Partition_t>> partition(const std::string &name, const std::string &from = "") override;

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
  bool hasPartition(const std::string &name) const override;
  /// @brief Check whether the partition table is available.
  bool hasTable(const std::string &name) const;
  /// @brief Check the device is uses UFS.
  bool isUsesUFS() const;
  /// @brief Check whether the super partition exists in any partition table.
  bool isHasSuperPartition() const;

  /// @brief Check whether the partition is logical.
  constexpr bool isLogical(const std::string &) const override { return false; }

  /// @brief Check whether the partition table is supported.
  bool isSupported() const noexcept override { return true; }

  /// @brief Checks @c partitions and @c gptDataCollection is empty.
  bool empty() const override;

  /// @brief Checks @c tableNames is empty.
  bool tableNamesEmpty() const;

  /// @brief Validate GPTData collection integrity. Checks with @c GPTData::Verify() and @c GPTData::CheckHeaderValidity().
  bool valid() const override;

  /// @brief For-each input function for all partitions (constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEach(F &&function) const {
    Log::info("Foreaching input function for all partitions.");
    bool isSuccess = true;
    for (auto &part : localPartitions)
      isSuccess &= function(part);

    return isSuccess;
  }

  /// @brief For-each input function for all partitions (non-constant).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEach(F &&function) {
    Log::info("Foreaching input function for all partitions.");
    bool isSuccess = true;
    for (auto &part : localPartitions)
      isSuccess &= function(part);

    return isSuccess;
  }

  /// @brief For-each input function for gpt data collection (constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const std::filesystem::path &, const std::shared_ptr<GPTData> &>
  bool forEachGptData(F &&function) const {
    Log::info("Foreaching input function for all GPTData data.");
    bool isSuccess = true;
    for (auto &[path, gptData] : gptDataCollection)
      isSuccess &= function(path, gptData);

    return isSuccess;
  }

  /// @brief For-each input function for gpt data collection (non-constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const std::filesystem::path &, std::shared_ptr<GPTData> &>
  bool forEachGptData(F &&function) {
    Log::info("Foreaching input function for all GPTData data.");
    bool isSuccess = true;
    for (auto &[path, gptData] : gptDataCollection)
      isSuccess &= function(path, gptData);

    return isSuccess;
  }

  /// @brief For-each input function for input partition list (constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEachFor(const std::vector<std::string> &list, F &&function) const {
    Log::info("Foreaching input function for input list.");
    bool isSuccess = true;
    for (auto &name : list) {
      if (hasPartition(name)) isSuccess &= function(partition(name)->get());
    }

    return isSuccess;
  }

  /// @brief For-each input function for input partition list (non-constant).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEachFor(const std::vector<std::string> &list, F &&function) {
    Log::info("Foreaching input function for input list.");
    bool isSuccess = true;
    for (auto &name : list) {
      if (hasPartition(name)) isSuccess &= function(partition(name)->get());
    }

    return isSuccess;
  }

  /// @brief Rescan tables.
  void reScan() override;
  /// @brief Removes table.
  void removeTable(const std::string &name);

  /// @brief Set tables by names (like @c sda, @c sdc, @c sdg ).
  void setTables(const std::unordered_set<std::string> &tables);

  /// @brief Set tables (chain function) by names (like @c sda, @c sdc, @c sdg).
  PartitionTableData &withTables(const std::unordered_set<std::string> &tables);

  /// @brief Change auto scanning state.
  void setAutoScan(bool state);
  /// @brief Change auto scanning state (chain function).
  PartitionTableData &withAutoScan(bool state);

  /// @brief Set GPTData of the table.
  void setGPTDataOf(const std::string &name, std::shared_ptr<GPTData> data);

  /// @brief Cleanup data (excepts auto scan state and seek name).
  void clear() override;
  /// @brief Cleanup (<tt>clear()</tt>) and reset variables.
  void reset() override;

  /**
   * @name PartitionTableData's iterators and operators.
   * @brief Iterator and operator functions of @c Builder.
   * @{
   */
  iterator begin() override;              ///< Non-const begin iterator for range-based loop.
  iterator end() override;                ///< Non-const end iterator for range-based loop.
  const_iterator begin() const override;  ///< Const begin iterator for range-based loop.
  const_iterator end() const override;    ///< Const end iterator for range-based loop.
  const_iterator cbegin() const override; ///< Const begin iterator for modern C++ range-based loop.
  const_iterator cend() const override;   ///< Const begin iterator for modern C++ range-based loop.

  bool operator==(const PartitionTableData &other) const; ///< @c == assignment.
  bool operator!=(const PartitionTableData &other) const; ///< @c != assignment.
  explicit operator bool() const override;                ///< Same as @c valid().
  bool operator!() const override;                        ///< Same as @c valid().

  const_list_t operator*() const; ///< <tt>std::vector<const Partition_t *> = *pd;</tt>
  list_t operator*();             ///< <tt>std::vector<Partition_t *> = *pd;</tt>
  const std::shared_ptr<GPTData> &
  operator[](const std::string &name) const;                     ///< <tt>const std::shared_ptr<GPTData> data = pd["sda"];</tt>
  std::shared_ptr<GPTData> &operator[](const std::string &name); ///< <tt>std::shared_ptr<GPTData> data = pd["sda"];</tt>
  const GPTPart *operator()(const std::string &name, uint32_t index) const; ///< <tt>const GPTPart* part = pd("sda", 3);</tt>
  GPTPart *operator()(const std::string &name, uint32_t index);             ///< <tt>GPTPart* part = pd("sda", 3);</tt>F

  PartitionTableData &operator=(const PartitionTableData &other) = default; ///< Copy assignment.
  PartitionTableData &operator=(PartitionTableData &&other) noexcept;       ///< Move assignment.

  /** @} */
}; // class PartitionTableData

/**
 * @brief A class that examines and neatly lists all logical partitions on the device. It records information about the logical
 * partitions, etc.
 * @note This class collects data using the Partition_t and LpMetadata class.
 * @see PartitionMap::Partition_t
 * @see android::fs_mgr::LpMetadata
 * @see [GPT fdisk](https://android.googlesource.com/platform/external/gptfdisk)
 * @see [liblp](https://android.googlesource.com/platform/system/core/+/refs/heads/main/fs_mgr/liblp)
 */
class DynamicTableData : public BaseTableData {
  std::vector<Partition_t> localPartitions;
  std::unique_ptr<android::fs_mgr::LpMetadata> lpMetadata;
  bool supported = false;

  void scan();

public:
  /// @brief List type.
  using list_t = std::vector<std::reference_wrapper<Partition_t>>;
  /// @brief Constant list type.
  using const_list_t = std::vector<std::reference_wrapper<const Partition_t>>;

  /// @brief Iterator.
  using iterator = std::vector<Partition_t>::iterator;
  /// @brief Constant iterator.
  using const_iterator = std::vector<Partition_t>::const_iterator;

  DynamicTableData() { scan(); }

  /// @brief Copy constructor.
  DynamicTableData(const DynamicTableData &other) {
    localPartitions = other.localPartitions;
    lpMetadata = std::make_unique<android::fs_mgr::LpMetadata>(*(other.lpMetadata));
  }

  /// @brief Move constructor.
  DynamicTableData(DynamicTableData &&other) noexcept
      : localPartitions(std::move(other.localPartitions)), lpMetadata(std::move(other.lpMetadata)) {}

  constexpr TableType type() const noexcept override { return TableType::DYNAMIC; }

  static DynamicTableData *cast(BaseTableData *base) {
    assert(dynamic_cast<DynamicTableData *>(base) != nullptr);
    return static_cast<DynamicTableData *>(base);
  }

  static const DynamicTableData *cast(const BaseTableData *base) {
    assert(dynamic_cast<const DynamicTableData *>(base) != nullptr);
    return static_cast<const DynamicTableData *>(base);
  }

  /// @brief Get references of partitions.
  list_t partitions() override;
  /// @brief Get references of partitions.
  const_list_t partitions() const override;

  /// @brief Get @c metadata.
  android::fs_mgr::LpMetadata &getMetadata();

  /// @brief Get @c metadata (const).
  const android::fs_mgr::LpMetadata &getMetadata() const;

  /// @brief Get partition groups.
  std::vector<LpMetadataPartitionGroup> &getGroups();

  /// @brief Get partition groups (const).
  const std::vector<LpMetadataPartitionGroup> &getGroups() const;

  /// @brief Get information about partitions.
  std::vector<BasicInfo> aboutPartitions() const override;

  /**
   * @brief Get Partition_t object of needed partition.
   * @param name Partition name.
   * @retval std::nullopt Partition not found.
   * @retval "std::reference_wrapper<const Partition_t>" Partition is found.
   */
  std::optional<std::reference_wrapper<const Partition_t>> partition(const std::string &name, const std::string & = "") const override;

  /**
   * @brief Get Partition_t object of needed partition.
   * @param name Partition name.
   * @retval std::nullopt Partition not found.
   * @retval std::reference_wrapper<Partition_t> Partition is found.
   */
  std::optional<std::reference_wrapper<Partition_t>> partition(const std::string &name, const std::string & = "") override;

  /**
   * @brief Get LpMetadataPartition of needed partition.
   * @param name Partition name.
   * @retval std::nullopt Partition not found.
   * @retval std::reference_wrapper<LpMetadataPartition> Partition is found.
   */
  std::optional<std::reference_wrapper<LpMetadataPartition>> metadata(const std::string &name);

  /**
   * @brief Get LpMetadataPartition of needed partition.
   * @param name Partition name.
   * @retval std::nullopt Partition not found.
   * @retval "std::reference_wrapper<const LpMetadataPartition>" Partition is found.
   */
  std::optional<std::reference_wrapper<const LpMetadataPartition>> metadata(const std::string &name) const;

  /// @brief Get free space of the super partition.
  uint64_t freeSpace() const;

  /// @brief Get free space of an group.
  uint64_t freeSpace(const std::string &name) const;

  /// @brief Get total size of the super partition.
  uint64_t size() const;

  /// @brief Get max size of an group.
  uint64_t size(const std::string &name);

  /// @brief Check the availability of the partition.
  bool hasPartition(const std::string &name) const override;

  /// @brief Check whether the partition is logical.
  constexpr bool isLogical(const std::string &) const override { return true; }

  /// @brief Check whether the partition table is supported.
  bool isSupported() const noexcept override { return supported; }

  /// @brief Checks @c partitions is empty.
  bool empty() const override;

  /// @brief Validate the metadata is not empty and etc.
  bool valid() const override;

  /// @brief Validate the metadata is not empty.
  bool validMetadata() const;

  /// @brief For-each input function for partitions (constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEach(F &&function) const {
    Log::info("Foreaching input function for all partitions.");
    bool isSuccess = true;
    for (auto &part : localPartitions)
      isSuccess &= function(part);

    return isSuccess;
  }

  /// @brief For-each input function for partition metadatas (constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const LpMetadataPartition &>
  bool forEach(F &&function) const {
    Log::info("Foreaching input function for all partition metadatas.");
    bool isSuccess = true;
    for (const auto &part : lpMetadata->partitions)
      isSuccess &= function(part);

    return isSuccess;
  }

  /// @brief For-each input function for partitions (non-constant).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEach(F &&function) {
    Log::info("Foreaching input function for all partitions.");
    bool isSuccess = true;
    for (auto &part : localPartitions)
      isSuccess &= function(part);

    return isSuccess;
  }
  /// @brief For-each input function for partition metadatas (non-constant).
  template <typename F>
    requires Helper::Invocable<F, bool, LpMetadataPartition &>
  bool forEach(F &&function) {
    Log::info("Foreaching input function for all partition metadatas.");
    bool isSuccess = true;
    for (auto &part : lpMetadata->partitions)
      isSuccess &= function(part);

    return isSuccess;
  }

  /// @brief For-each input function for input partition list (constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const Partition_t &>
  bool forEachFor(const std::vector<std::string> &list, F &&function) const {
    Log::info("Foreaching input function for input list.");
    bool isSuccess = true;
    for (auto &name : list) {
      if (hasPartition(name)) isSuccess &= function(partition(name)->get());
    }

    return isSuccess;
  }

  /// @brief For-each input function for input partition list metadata's (constant).
  template <typename F>
    requires Helper::Invocable<F, bool, const LpMetadataPartition &>
  bool forEachFor(const std::vector<std::string> &list, F &&function) const {
    Log::info("Foreaching input function for input list.");
    bool isSuccess = true;
    for (auto &name : list) {
      if (hasPartition(name)) isSuccess &= function(metadata(name)->get());
    }

    return isSuccess;
  }

  /// @brief For-each input function for input partition list (non-constant).
  template <typename F>
    requires Helper::Invocable<F, bool, Partition_t &>
  bool forEachFor(const std::vector<std::string> &list, F &&function) {
    Log::info("Foreaching input function for input list.");
    bool isSuccess = true;
    for (auto &name : list) {
      if (hasPartition(name)) isSuccess &= function(partition(name)->get());
    }

    return isSuccess;
  }

  /// @brief For-each input function for input partition list  metadata's (non-constant).
  template <typename F>
    requires Helper::Invocable<F, bool, LpMetadataPartition &>
  bool forEachFor(const std::vector<std::string> &list, F &&function) {
    Log::info("Foreaching input function for input list.");
    bool isSuccess = true;
    for (auto &name : list) {
      if (hasPartition(name)) isSuccess &= function(metadata(name)->get());
    }

    return isSuccess;
  }

  /// @brief Rescan tables.
  void reScan() override;
  /// @brief Cleanup data..
  void clear() override;
  /// @brief Same as (<tt>clear()</tt>).
  void reset() override;

  /**
   * @name DynamicTableData's iterators and operators.
   * @brief Iterator and operator functions of @c Builder.
   * @{
   */
  iterator begin() override;              ///< Non-const begin iterator for range-based loop.
  iterator end() override;                ///< Non-const end iterator for range-based loop.
  const_iterator begin() const override;  ///< Const begin iterator for range-based loop.
  const_iterator end() const override;    ///< Const end iterator for range-based loop.
  const_iterator cbegin() const override; ///< Const begin iterator for modern C++ range-based loop.
  const_iterator cend() const override;   ///< Const begin iterator for modern C++ range-based loop.

  bool operator==(const DynamicTableData &other) const; ///< @c == assignment.
  bool operator!=(const DynamicTableData &other) const; ///< @c != assignment.
  explicit operator bool() const override;              ///< Same as @c valid().
  bool operator!() const override;                      ///< Same as @c valid().

  const_list_t operator*() const; ///< <tt>std::vector<const Partition_t *> = *pd;</tt>
  list_t operator*();             ///< <tt>std::vector<Partition_t *> = *pd;</tt>

  DynamicTableData &operator=(const DynamicTableData &other);     ///< Copy assignment.
  DynamicTableData &operator=(DynamicTableData &&other) noexcept; ///< Move assignment.

  /** @} */
}; // class DynamicTableData

} // namespace PartitionMap

#endif // #ifndef LIBPARTITION_MAP_TABLE_DATA_COLLECTION_HPP
