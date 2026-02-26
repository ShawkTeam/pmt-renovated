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

#ifndef LIBPARTITION_MAP_DEFINATONS_HPP
#define LIBPARTITION_MAP_DEFINATONS_HPP

#if __cplusplus < 202002L
#error "libpartition_map/definations.hpp is requires C++20 or higher C++ standarts."
#endif

#include <filesystem>
#include <string>
#include <map>
#include <gpt.h>

#ifdef NONE
#undef NONE
#endif
#ifdef ERR
#undef ERR
#define ERR PartitionMap::Error()
#endif

namespace PartitionMap {
enum SizeUnit : int { BYTE = 1, KiB = 2, MiB = 3, GiB = 4 };

struct BasicData {
  GPTPart gptPart;
  uint32_t index;
  std::filesystem::path tablePath;
};
struct BasicInfo {
  std::string name;
  uint64_t size;
  bool isLogical;
};

template <typename __class>
concept minimumPartitionClass = requires(__class cls, __class cls2, GUIDData gdata, SizeUnit unit, uint32_t sector, bool no_throw,
                                         const BasicData &data, const std::filesystem::path &path) {
  // Check required functions
  { cls.path() } -> std::same_as<std::filesystem::path>;
  { cls.pathByName() } -> std::same_as<std::filesystem::path>;
  { cls.absolutePath() } -> std::same_as<std::filesystem::path>;
  { cls.name() } -> std::convertible_to<std::string>;
  { cls.formattedSizeString(unit, no_throw) } -> std::convertible_to<std::string>;
  { cls.size(sector) } -> std::same_as<uint64_t>;
  { cls.empty() } -> std::convertible_to<bool>;

  // Check required constructors
  __class{};
  __class{path};
  __class{data};
  __class{cls};
  __class(std::move(cls2));

  // Check required operators
  { cls == cls2 } -> std::convertible_to<bool>;
  { cls == gdata } -> std::convertible_to<bool>;
  { cls != cls2 } -> std::convertible_to<bool>;
  { cls != gdata } -> std::convertible_to<bool>;
  { static_cast<bool>(cls) } -> std::convertible_to<bool>;
  { !cls } -> std::convertible_to<bool>;
  cls = cls2;
  cls = std::move(cls2);
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

#endif // #ifndef LIBPARTITION_MAP_DEFINATONS_HPP
