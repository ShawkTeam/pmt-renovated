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

#ifndef LIBPARTITION_MAP_SUPER_HPP
#define LIBPARTITION_MAP_SUPER_HPP

#if __cplusplus < 202002L
#error "libpartition_map/super.hpp is requires C++20 or higher C++ standarts."
#endif

#include <string>
#include <memory>
#include <vector>
#include <filesystem>
#include <liblp/liblp.h>
#include <liblp/builder.h>
#include <android-base/properties.h>

namespace PartitionMap {

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

} // namespace PartitionMap

#endif // #ifndef LIBPARTITION_MAP_SUPER_HPP
