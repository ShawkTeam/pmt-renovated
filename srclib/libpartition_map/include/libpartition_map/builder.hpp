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
#include <optional>
#include <gpt.h>
#include <libhelper/definations.hpp>
#include <libpartition_map/table_data_collection.hpp>

namespace PartitionMap {

class Builder {
public:
  template <TableType type, typename... Args> static auto *create(Args &&...args) {
    if constexpr (type == TableType::CLASSIC) {
      return new PartitionTableData(std::forward<Args>(args)...);
    } else if constexpr (type == TableType::DYNAMIC) {
      return new DynamicTableData(std::forward<Args>(args)...);
    } else {
      static_assert(false, "Unknown TableType");
    }
  }

  template <TableType type> static BaseTableData *getInstance();

  template <> BaseTableData *getInstance<TableType::CLASSIC>() {
    static PartitionTableData instance;
    return &instance;
  }

  template <> BaseTableData *getInstance<TableType::DYNAMIC>() {
    static DynamicTableData instance;
    return &instance;
  }

  static std::pair<BaseTableData *, BaseTableData *> getInstances() {
    return {getInstance<TableType::CLASSIC>(), getInstance<TableType::DYNAMIC>()};
  }

  template <TableType type> static auto *cast(BaseTableData *base) {
    if constexpr (type == TableType::CLASSIC) {
      assert(dynamic_cast<PartitionTableData *>(base) != nullptr);
      return static_cast<PartitionTableData *>(base);
    } else if constexpr (type == TableType::DYNAMIC) {
      assert(dynamic_cast<DynamicTableData *>(base) != nullptr);
      return static_cast<DynamicTableData *>(base);
    } else {
      static_assert(false, "Unknown TableType");
    }
  }

  static const PartitionTableData *cast(const BaseTableData *base) {
    assert(dynamic_cast<const PartitionTableData *>(base) != nullptr);
    return static_cast<const PartitionTableData *>(base);
  }

  static std::pair<PartitionTableData *, DynamicTableData *> cast(BaseTableData *base1, BaseTableData *base2) {
    assert(dynamic_cast<PartitionTableData *>(base1) != nullptr);
    assert(dynamic_cast<DynamicTableData *>(base2) != nullptr);
    return {static_cast<PartitionTableData *>(base1), static_cast<DynamicTableData *>(base2)};
  }

  static std::pair<const PartitionTableData *, const DynamicTableData *> cast(const BaseTableData *base1, const BaseTableData *base2) {
    assert(dynamic_cast<const PartitionTableData *>(base1) != nullptr);
    assert(dynamic_cast<const DynamicTableData *>(base2) != nullptr);
    return {static_cast<const PartitionTableData *>(base1), static_cast<const DynamicTableData *>(base2)};
  }
}; // namespace Builder

inline BaseTableData *getCorrectTableObj(const std::string &name, BaseTableData *c1, BaseTableData *c2,
                                         std::optional<TableType> &tType) {
  if (!c1 || !c2) return nullptr;
  if (c1->hasPartition(name)) tType = c1->type();
  if (c2->hasPartition(name)) tType = c2->type();
  if (!tType.has_value()) return nullptr;
  if (tType == PartitionMap::TableType::CLASSIC) return c1;
  return c2;
}

inline Partition_t *setupPartition(const std::string &name, BaseTableData *table) {
  if (!table) return nullptr;
  Partition_t *partition = nullptr;
  if (table->type() == PartitionMap::CLASSIC)
    partition = &PartitionMap::Builder::cast<PartitionMap::CLASSIC>(table)->partitionWithDupCheck(name)->get();
  else if (table->type() == PartitionMap::DYNAMIC)
    partition = &table->partition(name)->get();
  return partition;
}

} // namespace PartitionMap

#endif // #ifndef LIBPARTITION_MAP_BUILDER_HPP
