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

#ifndef LIBPARTITION_MAP_DEFINATIONS_HPP
#define LIBPARTITION_MAP_DEFINATIONS_HPP

#if __cplusplus < 202002L
#error "libpartition_map/definations.hpp is requires C++20 or higher C++ standarts."
#endif

#include <filesystem>
#include <string>
#include <map>
#include <type_traits>
#include <gpt.h>

#ifdef NONE
#undef NONE
#endif

namespace PartitionMap {
enum SizeUnit : int { BYTE = 1, KiB = 2, MiB = 3, GiB = 4 };

template <typename slot_type>
  requires std::is_integral_v<slot_type>
struct basic_data_base {
  GPTPart gptPart;
  slot_type index;
  std::filesystem::path tablePath;
};

template <typename size_type>
  requires std::is_integral_v<size_type>
struct basic_info_base {
  std::string name;
  uint64_t size{};
  bool isLogical = false;
};

using BasicData = basic_data_base<uint32_t>;
using BasicInfo = basic_info_base<uint64_t>;

template <typename T>
concept IsStringOrPath = !std::is_reference_v<T> && (std::is_same_v<T, std::filesystem::path> || std::is_same_v<T, std::string>);

template <typename T>
concept IsSizeType = !std::is_reference_v<T> && std::is_unsigned_v<T> && (std::is_integral_v<T> || std::is_floating_point_v<T>);

template <typename T>
concept IsSlotType = !std::is_reference_v<T> && std::is_integral_v<T> && !std::is_floating_point_v<T>;

template <typename T, typename U>
concept is_size_type_decay = std::is_same_v<std::decay_t<T>, std::decay_t<U>> && IsSizeType<std::decay_t<T>>;

template <typename T, typename U>
concept is_slot_type_decay = std::is_same_v<std::decay_t<T>, std::decay_t<U>> && IsSlotType<std::decay_t<T>>;

template <typename T, typename U>
concept is_string_or_path_decay = std::is_same_v<std::decay_t<T>, std::decay_t<U>> && IsStringOrPath<std::decay_t<T>>;

template <typename T>
concept is_gptpart_decay = std::is_same_v<std::decay_t<T>, GPTPart>;

template <typename T>
concept IsPathTypeLike = requires(T v1, T v2, std::string s, const char *cp) {
  v1.append(s);
  v1.append(cp);
  { v1.filename() } -> IsStringOrPath;
  requires(std::same_as<std::decay_t<T>, std::filesystem::path>) || requires {
    { v1.string() } -> std::convertible_to<std::string>;
  };

  std::is_constructible_v<T>;
  std::is_constructible_v<T, std::string>;
  std::is_constructible_v<T, const char *>;
  std::is_nothrow_move_constructible_v<T>;

  v1 = v2;
  v1 == v2;
  v1 != v2;
  v1 = std::move(v2);
};

template <typename __class>
concept IsValidPartitionClass = requires(__class cls, __class cls2, GUIDData gdata, SizeUnit unit, uint32_t sector, bool no_throw) {
  // Check required functions
  { cls.path() } -> std::same_as<std::filesystem::path>;
  { cls.pathByName() } -> std::same_as<std::filesystem::path>;
  { cls.absolutePath() } -> std::same_as<std::filesystem::path>;
  { cls.name() } -> std::convertible_to<std::string>;
  { cls.formattedSizeString(unit, no_throw) } -> std::convertible_to<std::string>;
  { cls.size(sector) } -> std::same_as<uint64_t>;
  { cls.empty() } -> std::convertible_to<bool>;

  // Check required constructors, etc.
  std::is_constructible_v<__class>;
  std::is_constructible_v<__class, std::filesystem::path>;
  std::is_constructible_v<__class, const BasicData &>;
  std::is_copy_constructible_v<__class>;
  std::is_nothrow_move_constructible_v<__class>;

  // Check required operators
  { cls == cls2 } -> std::convertible_to<bool>;
  { cls == gdata } -> std::convertible_to<bool>;
  { cls != cls2 } -> std::convertible_to<bool>;
  { cls != gdata } -> std::convertible_to<bool>;
  { static_cast<bool>(cls) } -> std::convertible_to<bool>;
  { !cls } -> std::convertible_to<bool>;
  std::is_copy_assignable_v<__class>;
  std::is_nothrow_move_assignable_v<__class>;
}; // concept minimumPartitionClass

using Error = Helper::Error;

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

} // namespace Extra
} // namespace PartitionMap

// clang-format off
#define FOREACH_PARTITIONS_LAMBDA_PARAMETERS         (PartitionMap::Partition_t & partition)
#define FOREACH_PARTITIONS_LAMBDA_PARAMETERS_CONST   (const PartitionMap::Partition_t & partition)
#define FOREACH_GPT_DATA_LAMBDA_PARAMETERS           (const std::filesystem::path &path, std::shared_ptr<GPTData> &gptData)
#define FOREACH_GPT_DATA_LAMBDA_PARAMETERS_CONST     (const std::filesystem::path &path, const std::shared_ptr<GPTData> &gptData)
// clang-format on

#endif // #ifndef LIBPARTITION_MAP_DEFINATIONS_HPP
