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

#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <libhelper/lib.hpp>
#include <libpartition_map/lib.hpp>
#include <libpartition_map/redefine_logging_macros.hpp>

namespace PartitionMap::Extra {
std::map<uint64_t, std::string> FileSystemMagics = {
    {FileSystemMagic::EXTFS_FS, "EXT2/3/4"}, {FileSystemMagic::F2FS_FS, "F2FS"},   {FileSystemMagic::EROFS_FS, "EROFS"},
    {FileSystemMagic::EXFAT_FS, "exFAT"},    {FileSystemMagic::FAT12_FS, "FAT12"}, {FileSystemMagic::FAT16_FS, "FAT16"},
    {FileSystemMagic::FAT32_FS, "FAT32"},    {FileSystemMagic::NTFS_FS, "NTFS"},   {FileSystemMagic::MSDOS_FS, "MSDOS"}};

std::map<uint64_t, std::string> AndroidMagics = {{AndroidMagic::BOOT_IMAGE, "Android Boot Image"},
                                                 {AndroidMagic::VBOOT_IMAGE, "Android Vendor Boot Image"},
                                                 {AndroidMagic::LK_IMAGE, "Android LK (Bootloader)"},
                                                 {AndroidMagic::DTBO_IMAGE, "Android DTBO Image"},
                                                 {AndroidMagic::VBMETA_IMAGE, "Android VBMeta Image"},
                                                 {AndroidMagic::SUPER_IMAGE, "Android Super Image"},
                                                 {AndroidMagic::SPARSE_IMAGE, "Android Sparse Image"},
                                                 {AndroidMagic::ELF, "ELF"},
                                                 {AndroidMagic::RAW, "Raw Data"}};

std::map<uint64_t, std::string> Magics = {{AndroidMagic::BOOT_IMAGE, "Android Boot Image"},
                                          {AndroidMagic::VBOOT_IMAGE, "Android Vendor Boot Image"},
                                          {AndroidMagic::LK_IMAGE, "Android LK (Bootloader)"},
                                          {AndroidMagic::DTBO_IMAGE, "Android DTBO Image"},
                                          {AndroidMagic::VBMETA_IMAGE, "Android VBMeta Image"},
                                          {AndroidMagic::SUPER_IMAGE, "Android Super Image"},
                                          {AndroidMagic::SPARSE_IMAGE, "Android Sparse Image"},
                                          {AndroidMagic::ELF, "ELF"},
                                          {AndroidMagic::RAW, "Raw Data"},
                                          {FileSystemMagic::EXTFS_FS, "EXT2/3/4"},
                                          {FileSystemMagic::F2FS_FS, "F2FS"},
                                          {FileSystemMagic::EROFS_FS, "EROFS"},
                                          {FileSystemMagic::EXFAT_FS, "exFAT"},
                                          {FileSystemMagic::FAT12_FS, "FAT12"},
                                          {FileSystemMagic::FAT16_FS, "FAT16"},
                                          {FileSystemMagic::FAT32_FS, "FAT32"},
                                          {FileSystemMagic::NTFS_FS, "NTFS"},
                                          {FileSystemMagic::MSDOS_FS, "MSDOS"}};

size_t getMagicLength(const uint64_t magic) {
  size_t length = 0;
  for (int i = 0; i < 8; i++) {
    if ((magic >> (8 * i)) & 0xFF) length = i + 1;
  }
  return length;
}

bool hasMagic(const uint64_t magic, const ssize_t buf, const std::string &path) {
  LOGI << "Checking magic of " << path << " with using " << buf << " byte buffer size (has magic 0x" << std::hex << magic << "?)"
       << std::endl;
  Helper::garbageCollector collector;

  const int fd = Helper::openAndAddToCloseList(path, collector, O_RDONLY);
  if (fd < 0) return false;
  if (buf < 1) {
    LOGE << "Buffer size is older than 1" << std::endl;
    return false;
  }

  auto *buffer = new (std::nothrow) uint8_t[buf];
  collector.delAfterProgress(buffer);

  const ssize_t bytesRead = read(fd, buffer, buf);
  if (bytesRead < 0) return false;

  const size_t magicLength = getMagicLength(magic);
  if (magicLength == 0) return false;

  for (size_t i = 0; i <= bytesRead - magicLength; i++) {
    uint64_t value = 0;
    for (size_t j = 0; j < magicLength; ++j)
      value |= static_cast<uint64_t>(buffer[i + j]) << (8 * j);
    if (value == magic) {
      LOGI << path << " contains 0x" << std::hex << magic << std::endl;
      return true;
    }
  }

  LOGI << path << " is not contains 0x" << std::hex << magic << std::endl;
  return false;
}

std::string formatMagic(const uint64_t magic) {
  std::stringstream ss;
  ss << "0x" << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << magic;
  return ss.str();
}

std::string getSizeUnitAsString(SizeUnit size) {
  switch (size) {
    case BYTE:
      return "B";
    case KiB:
      return "KiB";
    case MiB:
      return "MiB";
    case GiB:
      return "GiB";
    default:
      return "";
  }
}

} // namespace PartitionMap::Extra
