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

#include "functions.hpp"
#include <PartitionManager/PartitionManager.hpp>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <future>
#include <unistd.h>

#define EFUN "eraseFunction"
#define FUNCTION_CLASS eraseFunction

namespace PartitionManager {
RUN_ASYNC(const std::string &partitionName, const uint64_t bufferSize) {
  if (!Variables->PartMap->hasPartition(partitionName))
    return {format("Couldn't find partition: %s", partitionName.data()), false};

  if (Variables->onLogical && !Variables->PartMap->isLogical(partitionName)) {
    if (Variables->forceProcess)
      LOGN(EFUN, WARNING)
          << "Partition " << partitionName
          << " is exists but not logical. Ignoring (from --force, -f)."
          << std::endl;
    else
      return {
          format("Used --logical (-l) flag but is not logical partition: %s",
                 partitionName.data()),
          false};
  }

  LOGN(EFUN, INFO) << "Using buffer size: " << bufferSize;

  // Automatically close file descriptors and delete allocated memories (arrays)
  Helper::garbageCollector collector;

  const int pfd = Helper::openAndAddToCloseList(
      Variables->PartMap->getRealPathOf(partitionName), collector, O_WRONLY);
  if (pfd < 0)
    return {format("Can't open partition: %s: %s", partitionName.data(),
                   strerror(errno)),
            false};

  if (!Variables->forceProcess)
    Helper::confirmPropt(
        "Are you sure you want to continue? This could render your device "
        "unusable! Do not continue if you "
        "do not know what you are doing!");

  LOGN(EFUN, INFO) << "Writing zero bytes to partition: " << partitionName
                   << std::endl;
  auto *buffer = new (std::nothrow) char[bufferSize];
  collector.delAfterProgress(buffer);
  memset(buffer, 0x00, bufferSize);

  ssize_t bytesWritten = 0;
  const uint64_t partitionSize = Variables->PartMap->sizeOf(partitionName);

  while (bytesWritten < partitionSize) {
    size_t toWrite = sizeof(buffer);
    if (partitionSize - bytesWritten < sizeof(buffer))
      toWrite = partitionSize - bytesWritten;

    if (const ssize_t result = write(pfd, buffer, toWrite); result == -1)
      return {format("Can't write zero bytes to partition: %s: %s",
                     partitionName.data(), strerror(errno)),
              false};
    else bytesWritten += result;
  }

  return {format("Successfully wrote zero bytes to the %s partition\n",
                 partitionName.data()),
          true};
}

INIT {
  LOGN(EFUN, INFO) << "Initializing variables of erase function." << std::endl;
  cmd = _app.add_subcommand("erase", "Writes zero bytes to partition(s)");
  cmd->add_option("partition(s)", partitions, "Partition name(s)")
      ->required()
      ->delimiter(',');
  cmd->add_option("-b,--buffer-size", bufferSize,
                  "Buffer size for writing zero bytes to partition(s)")
      ->transform(CLI::AsSizeValue(false))
      ->default_val("4KB");
  return true;
}

RUN {
  std::vector<std::future<pair>> futures;
  for (const auto &partitionName : partitions) {
    uint64_t buf = bufferSize;
    setupBufferSize(buf, partitionName);
    futures.push_back(
        std::async(std::launch::async, runAsync, partitionName, buf));
    LOGN(EFUN, INFO) << "Created thread for writing zero bytes to "
                     << partitionName << std::endl;
  }

  std::string end;
  bool endResult = true;
  for (auto &future : futures) {
    auto [fst, snd] = future.get();
    if (!snd) {
      end += fst + '\n';
      endResult = false;
    } else println("%s", fst.c_str());
  }

  if (!endResult) throw Error("%s", end.c_str());

  LOGN(EFUN, INFO) << "Operation successfully completed." << std::endl;
  return endResult;
}

IS_USED_COMMON_BODY

NAME { return EFUN; }
} // namespace PartitionManager
