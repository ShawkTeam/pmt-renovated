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
#include <unistd.h>

#include <PartitionManager/PartitionManager.hpp>
#include <cerrno>
#include <cstdlib>
#include <future>

#include "functions.hpp"

#define FFUN "flashFunction"
#define FUNCTION_CLASS flashFunction

namespace PartitionManager {
RUN_ASYNC(const std::string &partitionName, const std::string &imageName,
          const uint64_t bufferSize, const bool deleteAfterProgress) {
  if (!Helper::fileIsExists(imageName))
    return PairError("Couldn't find image file: %s", imageName.data());
  if (!PARTS.hasPartition(partitionName))
    return PairError("Couldn't find partition: %s", partitionName.data());
  if (Helper::fileSize(imageName) > PARTS.sizeOf(partitionName))
    return PairError("%s is larger than %s partition size!", imageName.data(),
                     partitionName.data());

  LOGN(FFUN, INFO) << "flashing " << imageName << " to " << partitionName << std::endl;

  if (VARS.onLogical && !PARTS.isLogical(partitionName)) {
    if (VARS.forceProcess)
      LOGN(FFUN, WARNING) << "Partition " << partitionName
                          << " is exists but not logical. Ignoring (from --force, -f)."
                          << std::endl;
    else
      return PairError("Used --logical (-l) flag but is not logical partition: %s",
                       partitionName.data());
  }

  LOGN(FFUN, INFO) << "Using buffer size: " << bufferSize << std::endl;

  // Automatically close file descriptors and delete allocated memories (arrays)
  Helper::garbageCollector collector;

  const int ffd = Helper::openAndAddToCloseList(imageName, collector, O_RDONLY);
  if (ffd < 0)
    return PairError("Can't open image file %s: %s", imageName.data(), strerror(errno));

  const int pfd = Helper::openAndAddToCloseList(PARTS.getRealPathOf(partitionName),
                                                collector, O_RDWR | O_TRUNC);
  if (pfd < 0)
    return PairError("Can't open partition: %s: %s", partitionName.data(),
                     strerror(errno));

  LOGN(FFUN, INFO) << "Writing image " << imageName << " to partition: " << partitionName
                   << std::endl;
  auto *buffer = new (std::nothrow) char[bufferSize];
  collector.delAfterProgress(buffer);
  memset(buffer, 0x00, bufferSize);

  ssize_t bytesRead;
  while ((bytesRead = read(ffd, buffer, bufferSize)) > 0) {
    if (const ssize_t bytesWritten = write(pfd, buffer, bytesRead);
        bytesWritten != bytesRead)
      return PairError("Can't write partition to output file %s: %s", imageName.data(),
                       strerror(errno));
  }

  if (deleteAfterProgress) {
    LOGN(FFUN, INFO) << "Deleting flash file: " << imageName << std::endl;
    if (!Helper::eraseEntry(imageName) && !VARS.quietProcess)
      WARNING(std::string("Cannot erase flash file: " + imageName + "\n").data());
  }

  return PairSuccess("%s is successfully wrote to %s partition", imageName.data(),
                     partitionName.data());
}

INIT {
  LOGN(FFUN, INFO) << "Initializing variables of flash function." << std::endl;
  cmd = _app.add_subcommand("flash", "Flash image(s) to partition(s)");
  cmd->add_option("partition(s)", rawPartitions, "Partition name(s)")->required();
  cmd->add_option("imageFile(s)", rawImageNames, "Name(s) of image file(s)")->required();
  cmd->add_option("-b,--buffer-size", bufferSize,
                  "Buffer size for reading image(s) and writing to partition(s)")
      ->transform(CLI::AsSizeValue(false))
      ->default_val("4KB");
  cmd->add_option("-I,--image-directory", imageDirectory,
                  "Directory to find image(s) and flash to partition(s)");
  cmd->add_flag("-d,--delete", deleteAfterProgress,
                "Delete flash file(s) after progress.")
      ->default_val(false);

  return true;
}

RUN {
  processCommandLine(partitions, imageNames, rawPartitions, rawImageNames, ',', true);
  if (partitions.size() != imageNames.size())
    throw CLI::ValidationError(
        "You must provide an image file(s) as long as the partition name(s)");

  for (size_t i = 0; i < partitions.size(); i++) {
    if (!imageDirectory.empty())
      imageNames[i].insert(0, imageDirectory + '/');
  }

  std::vector<std::future<pair>> futures;
  for (size_t i = 0; i < partitions.size(); i++) {
    uint64_t buf = bufferSize;

    setupBufferSize(buf, imageNames[i]);
    futures.push_back(std::async(std::launch::async, runAsync, partitions[i],
                                 imageNames[i], bufferSize, deleteAfterProgress));
    LOGN(FFUN, INFO) << "Created thread for flashing image to " << partitions[i]
                     << std::endl;
  }

  std::string end;
  bool endResult = true;
  for (auto &future : futures) {
    auto [fst, snd] = future.get();
    if (!snd) {
      end += fst + '\n';
      endResult = false;
    } else
      OUT.println("%s", fst.c_str());
  }

  if (!endResult)
    throw Error("%s", end.c_str());

  LOGN(FFUN, INFO) << "Operation successfully completed." << std::endl;
  return endResult;
}

IS_USED_COMMON_BODY

NAME { return FFUN; }
} // namespace PartitionManager
