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

#include <fcntl.h>
#include <libhelper/lib.hpp>
#include <libpartition_map/lib.hpp>
#include <sstream>
#include <string>
#include <unistd.h>
#include <unordered_map>

#include "PartitionManager/PartitionManager.hpp"

namespace PartitionMap::Extras {
std::unordered_map<uint64_t, std::string> FileSystemMagicMap = {
    {FileSystemMagic::EXTFS_FS, "EXT2/3/4"},
    {FileSystemMagic::F2FS_FS, "F2FS"},
    {FileSystemMagic::EROFS_FS, "EROFS"},
    {FileSystemMagic::EXFAT_FS, "exFAT"},
    {FileSystemMagic::FAT12_FS, "FAT12"},
    {FileSystemMagic::FAT16_FS, "FAT16"},
    {FileSystemMagic::FAT32_FS, "FAT32"},
    {FileSystemMagic::NTFS_FS, "NTFS"},
    {FileSystemMagic::MSDOS_FS, "MSDOS"}};

std::unordered_map<uint64_t, std::string> AndroidMagicMap = {
    {AndroidMagic::BOOT_IMAGE, "Android Boot Image"},
    {AndroidMagic::VBOOT_IMAGE, "Android Vendor Boot Image"},
    {AndroidMagic::LK_IMAGE, "Android LK (Bootloader)"},
    {AndroidMagic::DTBO_IMAGE, "Android DTBO Image"},
    {AndroidMagic::VBMETA_IMAGE, "Android VBMeta Image"},
    {AndroidMagic::SUPER_IMAGE, "Android Super Image"},
    {AndroidMagic::SPARSE_IMAGE, "Android Sparse Image"},
    {AndroidMagic::ELF, "ELF"},
    {AndroidMagic::RAW, "Raw Data"}};

std::unordered_map<uint64_t, std::string> MagicMap = {
    {AndroidMagic::BOOT_IMAGE, "Android Boot Image"},
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

bool hasMagic(const uint64_t magic, const ssize_t buf,
              const std::string &path) {
  LOGN(MAP, INFO) << "Checking magic of " << path << " with using " << buf
                  << " byte buffer size (has magic 0x" << std::hex << magic
                  << "?)" << std::endl;
  Helper::garbageCollector collector;

  const int fd = Helper::openAndAddToCloseList(path, collector, O_RDONLY);
  if (fd < 0) return false;
  if (buf < 1) {
    LOGN(MAP, ERROR) << "Buffer size is older than 1" << std::endl;
    return false;
  }

  auto *buffer = new(std::nothrow) uint8_t[buf];
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
      LOGN(MAP, INFO) << path << " contains 0x" << std::hex << magic
                      << std::endl;
      return true;
    }
  }

  LOGN(MAP, INFO) << path << " is not contains 0x" << std::hex << magic
                  << std::endl;
  return false;
}

std::string formatMagic(const uint64_t magic) {
  std::stringstream ss;
  ss << "0x" << std::uppercase << std::hex << std::setw(16) << std::setfill('0')
     << magic;
  return ss.str();
}
} // namespace PartitionMap::Extras
