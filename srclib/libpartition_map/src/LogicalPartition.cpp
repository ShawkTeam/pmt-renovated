/*
Copyright 2026 Yağız Zengin

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

#include <asm-generic/fcntl.h>
#include <filesystem>
#include <iomanip>
#include <libhelper/lib.hpp>
#include <libpartition_map/lib.hpp>
#include <libpartition_map/redefine_logging_macros.hpp>
#include <sys/ioctl.h>

namespace PartitionMap {

bool LogicalPartition_t::Extra::isReallyLogical(const std::filesystem::path &path) {
  return path.string().find("/mapper/") != std::string::npos;
}

std::filesystem::path LogicalPartition_t::getPath() const { return partitionPath; }
std::filesystem::path LogicalPartition_t::getAbsolutePath() const {
  return std::filesystem::read_symlink(partitionPath);
}

std::string LogicalPartition_t::getName() const {
  return partitionPath.filename().string();
}

uint64_t LogicalPartition_t::getSize() const {
  std::filesystem::path p = std::filesystem::read_symlink(partitionPath);
  Helper::garbageCollector collector;
  int fd = Helper::openAndAddToCloseList(p.string(), collector, O_RDONLY);

  if (fd < 0) {
    LOGE << "Cannot open partition file path: " << std::quoted(partitionPath.string())
         << ": " << strerror(errno) << std::endl;
    return 0;
  }

  uint64_t size = 0;
  if (ioctl(fd, static_cast<unsigned int>(BLKGETSIZE64), &size) != 0) {
    LOGN(MAP, ERROR) << "ioctl(BLKGETSIZE64) failed for "
                     << std::quoted(partitionPath.string()) << ": " << strerror(errno)
                     << std::endl;
    return 0;
  }

  return size;
}

bool LogicalPartition_t::empty() const { return partitionPath.empty(); }

void LogicalPartition_t::setPartitionPath(const std::filesystem::path &path) {
  partitionPath = path;
}

bool LogicalPartition_t::operator==(const LogicalPartition_t &other) const {
  return partitionPath == other.partitionPath;
}

bool LogicalPartition_t::operator!=(const LogicalPartition_t &other) const {
  return partitionPath != other.partitionPath;
}

LogicalPartition_t::operator bool() const { return !empty(); }

bool LogicalPartition_t::operator!() const { return empty(); }

} // namespace PartitionMap
