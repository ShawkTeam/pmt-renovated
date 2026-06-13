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
 * @file definations.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Some concept definitions, descriptions, etc. of the library.
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

/**
 * @namespace PartitionMap
 * @brief Main namespace of libpartition_map library.
 */
namespace PartitionMap {

/// @brief Generic size type (arch-spefic).
#ifdef __LP64__
using GenericSizeType = uint64_t;
#else
using GenericSizeType = uint32_t;
#endif

/// @brief /// @brief Short names used in dimension type conversions.
enum SizeUnit : int { BYTE = 1, KiB = 2, MiB = 3, GiB = 4 };

enum TableType : int { DYNAMIC = 1, CLASSIC = 2 };

/// @brief A struct that holds basic data about a partition.
template <typename slot_type>
  requires std::is_integral_v<slot_type>
struct basic_data_base {
  GPTPart gptPart;                 ///< @c GPTPart object.
  slot_type index;                 ///< Index of partition.
  std::filesystem::path tablePath; ///< Table path.
};

/// @brief Data structure used when providing information about the partition.
template <typename size_type>
  requires std::is_integral_v<size_type>
struct basic_info_base {
  std::string name;       ///< Partition name.
  uint64_t size{};        ///< Partition size.
  bool isLogical = false; ///< Partition is logical or not.
};

using BasicData = basic_data_base<uint32_t>;
using BasicInfo = basic_info_base<uint64_t>;

/**
 * @brief Verify that the type is @c std::string or @c std::filesystem::path.
 * @tparam T Type.
 * @note References are not accepted.
 */
template <typename T>
concept IsStringOrPath = !std::is_reference_v<T> && (std::is_same_v<T, std::filesystem::path> || std::is_same_v<T, std::string>);

/**
 * @brief Verify that the type is a size type (unsigned, integral or floating point).
 * @tparam T Type.
 * @note References are not accepted.
 */
template <typename T>
concept IsSizeType = !std::is_reference_v<T> && std::is_unsigned_v<T> && (std::is_integral_v<T> || std::is_floating_point_v<T>);

/**
 * @brief Verify that the type is suitable for holding the slot number (unsigned integrals, non-floating point).
 * @tparam T Type.
 * @note References are not accepted.
 */
template <typename T>
concept IsSlotType = !std::is_reference_v<T> && std::is_integral_v<T> && !std::is_floating_point_v<T>;

/**
 * @namespace FindInArgs
 * @brief The namespace containing the concepts that perform the relevant searches within the template argument package.
 */
namespace FindInArgs {

/**
 * @brief Verify that the argument package has a size type (unsigned, integral or floating point).
 * @tparam Args Template argument package.
 */
template <typename... Args>
concept HasSizeType = (IsSizeType<std::decay_t<Args>> || ...);

/**
 * @brief Verify that the argument package has type suitable for holding the slot number (unsigned integrals, non-floating point).
 * @tparam Args Template argument package.
 */
template <typename... Args>
concept HasSlotType = (IsSlotType<std::decay_t<Args>> || ...);

/**
 * @brief Verify that the argument package has a @c std::string or @c std::filesystem::path type.
 * @tparam Args Template argument package.
 */
template <typename... Args>
concept HasStringOrPath = (IsStringOrPath<std::decay_t<Args>> || ...);

/**
 * @brief Verify that the argument package has a @c GPTPart type.
 * @tparam Args Template argument package.
 */
template <typename... Args>
concept HasGPTPart = (std::is_same_v<std::decay_t<Args>, GPTPart> || ...);

} // namespace FindInArgs

/// @brief Verify that the type has similar properties to @c std::filesystem::path.
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

/// @brief It checks that the type meets the requirements to be the partition class.
template <typename Class>
concept IsValidPartitionClass = requires(Class cls, Class cls2, GUIDData gdata, SizeUnit unit, uint32_t sector, bool no_throw) {
  // Check required functions
  { cls.path() } -> std::same_as<std::filesystem::path>;
  { cls.pathByName() } -> std::same_as<std::filesystem::path>;
  { cls.absolutePath() } -> std::same_as<std::filesystem::path>;
  { cls.name() } -> std::convertible_to<std::string>;
  { cls.formattedSizeString(unit, no_throw) } -> std::convertible_to<std::string>;
  { cls.size(sector) } -> std::integral;
  { cls.empty() } -> std::convertible_to<bool>;

  // Check required constructors, etc.
  std::is_constructible_v<Class>;
  std::is_constructible_v<Class, std::filesystem::path>;
  std::is_constructible_v<Class, const BasicData &>;
  std::is_copy_constructible_v<Class>;
  std::is_nothrow_move_constructible_v<Class>;

  // Check required operators
  { cls == cls2 } -> std::convertible_to<bool>;
  { cls == gdata } -> std::convertible_to<bool>;
  { cls != cls2 } -> std::convertible_to<bool>;
  { cls != gdata } -> std::convertible_to<bool>;
  { static_cast<bool>(cls) } -> std::convertible_to<bool>;
  { !cls } -> std::convertible_to<bool>;
  std::is_copy_assignable_v<Class>;
  std::is_nothrow_move_assignable_v<Class>;
}; // concept minimumPartitionClass

using Error = Helper::Error;

namespace Extra {

/// @brief Known magics of filesystems.
namespace FileSystemMagic {
inline constexpr uint64_t EXTFS_FS = 0xEF53;
inline constexpr uint64_t F2FS_FS = 0xF2F52010;
inline constexpr uint64_t EROFS_FS = 0xE0F5E1E2;
inline constexpr uint64_t EXFAT_FS = 0x5441465845;
inline constexpr uint64_t FAT12_FS = 0x3231544146;
inline constexpr uint64_t FAT16_FS = 0x3631544146;
inline constexpr uint64_t FAT32_FS = 0x3233544146;
inline constexpr uint64_t NTFS_FS = 0x5346544E;
inline constexpr uint64_t MSDOS_FS = 0x4d44;
} // namespace FileSystemMagic

/// @brief Known magics of android-spefic structures.
namespace AndroidMagic {
inline constexpr uint64_t BOOT_IMAGE = 0x2144494F52444E41;
inline constexpr uint64_t VBOOT_IMAGE = 0x544F4F4252444E56;
inline constexpr uint64_t LK_IMAGE = 0x00006B6C;
inline constexpr uint64_t DTBO_IMAGE = 0x1EABB7D7;
inline constexpr uint64_t VBMETA_IMAGE = 0x425641;
inline constexpr uint64_t SUPER_IMAGE = 0x61446C67;
inline constexpr uint64_t SPARSE_IMAGE = 0x3AFF26ED;
inline constexpr uint64_t DDR_IMAGE = 0x00524444;
inline constexpr uint64_t ZTECFG = 0x7A7465636667;
inline constexpr uint64_t ELF = 0x464C457F;
inline constexpr uint64_t RAW = 0x00000000;
} // namespace AndroidMagic

extern std::map<uint64_t, std::string> FileSystemMagics;
extern std::map<uint64_t, std::string> AndroidMagics;
extern std::map<uint64_t, std::string> Magics;

} // namespace Extra
} // namespace PartitionMap

// clang-format off
#define FOREACH_PARTITIONS_LAMBDA_PARAMETERS            (PartitionMap::Partition_t & partition)
#define FOREACH_PARTITIONS_LAMBDA_PARAMETERS_CONST      (const PartitionMap::Partition_t & partition)
#define FOREACH_LP_METADATA_PARTITION_PARAMETERS        (LpMetadataPartition & metadata)
#define FOREACH_LP_METADATA_PARTITION_PARAMETERS_CONST  (const LpMetadataPartition & metadata)
#define FOREACH_GPT_DATA_LAMBDA_PARAMETERS              (const std::filesystem::path &path, std::shared_ptr<GPTData> &gptData)
#define FOREACH_GPT_DATA_LAMBDA_PARAMETERS_CONST        (const std::filesystem::path &path, const std::shared_ptr<GPTData> &gptData)
// clang-format on

#endif // #ifndef LIBPARTITION_MAP_DEFINATIONS_HPP
