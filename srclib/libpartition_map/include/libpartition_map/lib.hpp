/*
   Copyright 2025 Yağız Zengin

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

#include <cstdint> // for uint64_t
#include <exception>
#include <functional>
#include <libhelper/lib.hpp>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility> // for std::pair

namespace PartitionMap {
struct _entry {
  std::string name;

  struct {
    uint64_t size;
    bool isLogical;
  } props;
};

struct _returnable_entry {
  uint64_t size;
  bool isLogical;
};

using BasicInf = _returnable_entry;
using Info = _entry;

/**
 *   The main type of the library. The Builder class is designed
 *   to be easily manipulated and modified only on this class.
 */
class basic_partition_map {
private:
  void _resize_map();

  [[nodiscard]] int _index_of(std::string_view name) const;

public:
  _entry *_data;
  size_t _count{}, _capacity{};

  basic_partition_map(const std::string &name, uint64_t size, bool logical);
  basic_partition_map(const basic_partition_map &other);
  basic_partition_map(basic_partition_map &&other) noexcept;
  basic_partition_map();
  ~basic_partition_map();

  bool insert(const std::string &name, uint64_t size, bool logical);
  void merge(const basic_partition_map &map);
  void clear();

  [[nodiscard]] uint64_t get_size(std::string_view name) const;
  [[nodiscard]] bool is_logical(std::string_view name) const;
  [[nodiscard]] _returnable_entry get_all(std::string_view name) const;
  [[nodiscard]] bool find(std::string_view name) const;
  [[nodiscard]] std::string find_(const std::string &name) const;
  [[nodiscard]] size_t size() const;
  [[nodiscard]] bool empty() const;

  basic_partition_map &operator=(const basic_partition_map &map);

  bool operator==(const basic_partition_map &other) const;
  bool operator!=(const basic_partition_map &other) const;
  bool operator!() const;
  explicit operator bool() const;

  Info operator[](int index) const;
  BasicInf operator[](const std::string_view &name) const;

  operator std::vector<Info>() const;
  operator int() const;

  class iterator {
  public:
    _entry *ptr;

    explicit iterator(_entry *p);

    auto operator*() const
        -> std::pair<std::string &, decltype(_entry::props) &>;
    _entry *operator->() const;
    iterator &operator++();
    iterator operator++(int);
    bool operator!=(const iterator &other) const;
    bool operator==(const iterator &other) const;
  };

  class constant_iterator {
  public:
    const _entry *ptr;

    explicit constant_iterator(const _entry *p);

    auto operator*() const
        -> std::pair<const std::string &, const decltype(_entry::props) &>;
    const _entry *operator->() const;
    constant_iterator &operator++();
    constant_iterator operator++(int);
    bool operator!=(const constant_iterator &other) const;
    bool operator==(const constant_iterator &other) const;
  };

  /* for-each support */
  [[nodiscard]] iterator begin() const;
  [[nodiscard]] iterator end() const;
  [[nodiscard]] constant_iterator cbegin() const;
  [[nodiscard]] constant_iterator cend() const;
};

using Partition_t = _entry;
using Map_t = basic_partition_map;

class basic_partition_map_builder final {
private:
  Map_t _current_map;
  std::string _workdir;
  bool _any_generating_error, _map_builded;

  Map_t _build_map(std::string_view path, bool logical = false);

  void _insert_logicals(Map_t &&logicals);
  void _map_build_check() const;

  [[nodiscard]] static bool _is_real_block_dir(std::string_view path);
  [[nodiscard]] uint64_t _get_size(const std::string &path);

public:
  /**
   *   By default, it searches the directories in the
   *   defaultEntryList in PartitionMap.cpp in order and
   *   uses the directory it finds.
   */
  basic_partition_map_builder();

  /**
   *   A constructor with input. Need search path
   */
  explicit basic_partition_map_builder(std::string_view path);

  /**
   *   Move constructor
   */
  basic_partition_map_builder(basic_partition_map_builder &&other) noexcept;

  /**
   *   Returns the current list content in Map_t type.
   *   If no list is created, returns std::nullopt.
   */
  [[nodiscard]] Map_t getAll() const;

  /**
   *   Returns information of a specific partition in
   *   Map_temp_t type. If the partition is not in the
   *   currently created list, returns std::nullopt.
   */
  [[nodiscard]] std::optional<std::pair<uint64_t, bool>>
  get(std::string_view name) const;

  /**
   *   If there is a logical partition(s) in the created
   *   list, it returns a list of type std::list (containing
   *   data of type std::string). If there is no logical
   *   partition in the created list, it returns std::nullopt.
   */
  [[nodiscard]] std::optional<std::list<std::string>>
  getLogicalPartitionList() const;

  /**
   *   The physical partitions in the created list are
   *   returned as std::list type. If there is no content
   *   due to any problem, returns std::nullopt.
   */
  [[nodiscard]] std::optional<std::list<std::string>>
  getPhysicalPartitionList() const;

  /**
   *   The partitions in the created list are returned as std::list
   *   If there is no content due to any problem, returns std::nullopt
   */
  [[nodiscard]] std::optional<std::list<std::string>> getPartitionList() const;

  /**
   *   Returns the full link path of the entered partition
   *   name in the current search directory as std::string.
   *   If the partition is not in the list, an empty
   *   std::string is returned.
   */
  [[nodiscard]] std::string getRealLinkPathOf(std::string_view name) const;

  /**
   *   Returns the actual path of the partition as
   *   std::string. Like /dev/block/sda5
   */
  [[nodiscard]] std::string getRealPathOf(std::string_view name) const;

  /**
   *   If it exists, the path to the search string is
   *   returned as std::string. If it does not exist,
   *   an empty std::string is returned.
   */
  [[nodiscard]] std::string getCurrentWorkDir() const;

  /**
   *   Returns whether the entered partition name is in the
   *   created partition list as a bool.
   */
  [[nodiscard]] bool hasPartition(std::string_view name) const;

  /**
   *   Returns true if the device has dynamic partitions, false otherwise.
   */
  [[nodiscard]] bool hasLogicalPartitions() const;

  /**
   *   Returns the bool type status of whether the
   *   entered partition name is marked as logical in the
   *   created list. Alternatively, the current partition
   *   information can be retrieved with the get() function
   *   and checked for logicality.
   */
  [[nodiscard]] bool isLogical(std::string_view name) const;

  /**
   *   Copy partition list to vec, current vec contents are cleaned
   */
  bool copyPartitionsToVector(std::vector<std::string> &vec) const;

  /**
   *   Copy logical partition list to vec, current vec contents are cleaned
   */
  bool copyLogicalPartitionsToVector(std::vector<std::string> &vec) const;

  /**
   *   Copy physical partition list to vec, current vec contents are cleaned
   */
  bool copyPhysicalPartitionsToVector(std::vector<std::string> &vec) const;

  /**
   *   The created list and the current search index name are cleared.
   */
  void clear();

  /**
   *   Do input function (lambda) for all partitions.
   */
  bool doForAllPartitions(
      const std::function<bool(std::string, BasicInf)> &func) const;

  /**
   *   Do input function (lambda) for physical partitions.
   */
  bool doForPhysicalPartitions(
      const std::function<bool(std::string, BasicInf)> &func) const;

  /**
   *   Do input function (lambda) for logical partitions.
   */
  bool doForLogicalPartitions(
      const std::function<bool(std::string, BasicInf)> &func) const;

  /**
   *   Do input function (lambda) for input partition list.
   */
  bool doForPartitionList(
      const std::vector<std::string> &partitions,
      const std::function<bool(std::string, BasicInf)> &func) const;

  /**
   *   The entered path is defined as the new search
   *   directory and the search is performed in the entered
   *   directory. If everything goes well, true is returned.
   */
  bool readDirectory(std::string_view path);

  /**
   *   Reads default /dev entries and builds map.
   */
  bool readDefaultDirectories();

  /**
   *   Whether the current list is empty or not is returned
   *   as bool type. If there is content in the list, true
   *   is returned, otherwise false is returned.
   */
  [[nodiscard]] bool empty() const;

  /**
   *   If it exists, the size of the partition with the
   *   entered name is returned as uint64_t type.
   *   If it does not exist, 0 is returned.
   */
  [[nodiscard]] uint64_t sizeOf(std::string_view name) const;

  /**
   *   If the content lists of the two created objects are
   *   the same (checked only according to the partition
   *   names), true is returned, otherwise false is returned
   */
  friend bool operator==(const basic_partition_map_builder &lhs,
                         const basic_partition_map_builder &rhs);

  /**
   *   The opposite logic of the == operator.
   */
  friend bool operator!=(const basic_partition_map_builder &lhs,
                         const basic_partition_map_builder &rhs);

  /**
   *   You can check whether the object was created
   *   successfully. If the problem did not occur, true is
   *   returned, if it did, false is returned.
   */
  explicit operator bool() const;

  /**
   *   Returns true if the object creation failed (i.e., there's a problem),
   *   and false if the object is correctly created.
   */
  bool operator!() const;

  /**
   *   Build map with input path. Implementation of readDirectory().
   */
  bool operator()(std::string_view path);

  /**
   *   Get Map_t object reference
   */
  Map_t &operator*();

  /**
   *   Get constant Map_t object reference
   */
  const Map_t &operator*() const;

  /**
   *  Get Info structure with given index
   */
  Info operator[](int index) const;

  /**
   *  Get BasicInfo structure with given index
   */
  BasicInf operator[](const std::string_view &name) const;

  /**
   *   Get map contents as vector (PartitionManager::Info type).
   */
  [[nodiscard]] operator std::vector<Info>() const;

  /**
   *   Get total partition count in map (int type).
   */
  [[nodiscard]] operator int() const;

  /**
   *   Get current working directory.
   */
  [[nodiscard]] operator std::string() const;
};

using Error = Helper::Error;

/**
 *   To get the version information of libpartition_map
 *   library. It is returned as std::string type.
 */
std::string getLibVersion();

using BuildMap = basic_partition_map_builder;
using Map = basic_partition_map_builder;

namespace Extras {
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
constexpr uint64_t SUPER_IMAGE = 0x7265797573;
constexpr uint64_t SPARSE_IMAGE = 0x3AFF26ED;
constexpr uint64_t ELF =
    0x464C457F; // It makes more sense than between file systems
constexpr uint64_t RAW = 0x00000000;
} // namespace AndroidMagic

extern std::map<uint64_t, std::string> FileSystemMagicMap;
extern std::map<uint64_t, std::string> AndroidMagicMap;
extern std::map<uint64_t, std::string> MagicMap;

size_t getMagicLength(uint64_t magic);
bool hasMagic(uint64_t magic, ssize_t buf, const std::string &path);
std::string formatMagic(uint64_t magic);
} // namespace Extras
} // namespace PartitionMap

#define MAP "libpartition_map"

#define COMMON_LAMBDA_PARAMS                                                   \
  (const std::string &partition, const PartitionMap::BasicInf props)

#endif // #ifndef LIBPARTITION_MAP_LIB_HPP
