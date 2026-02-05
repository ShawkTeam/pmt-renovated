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

#include <cerrno>
#include <cstdlib>
#include <future>
#include <fcntl.h>
#include <unistd.h>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <CLI11.hpp>

#define PLUGIN "FlashPlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class FlashPlugin final : public BasicPlugin {
  std::vector<std::string> partitions, imageNames;
  std::string rawPartitions, rawImageNames, imageDirectory;
  uint64_t bufferSize = 0;
  bool deleteAfterProgress = false;

public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;
  const char *logPath = nullptr;

  ~FlashPlugin() override = default;

  bool onLoad(CLI::App &mainApp, const std::string &logpath, FlagsBase &mainFlags) override {
    logPath = logpath.c_str();
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    flags = mainFlags;
    cmd = mainApp.add_subcommand("flash", "Flash image(s) to partition(s)");
    cmd->add_option("partition(s)", rawPartitions, "Partition name(s)")->required();
    cmd->add_option("imageFile(s)", rawImageNames, "Name(s) of image file(s)")->required();
    cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for reading image(s) and writing to partition(s)")
        ->transform(CLI::AsSizeValue(false))
        ->default_val("4KB");
    cmd->add_option("-I,--image-directory", imageDirectory, "Directory to find image(s) and flash to partition(s)");
    cmd->add_flag("-d,--delete", deleteAfterProgress, "Delete flash file(s) after progress.")->default_val(false);

    return true;
  }

  bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  bool used() override { return cmd->parsed(); }

  resultPair runAsync(const std::string &partitionName, const std::string &imageName) const {
    if (!Helper::fileIsExists(imageName)) return PairError("Couldn't find image file: %s", imageName.data());
    if (!TABLES.hasPartition(partitionName)) return PairError("Couldn't find partition: %s", partitionName.data());
    if (Helper::fileSize(imageName) > TABLES.partition(partitionName).getSize())
      return PairError("%s is larger than %s partition size!", imageName.data(), partitionName.data());

    uint64_t buf = bufferSize;
    setupBufferSize(buf, TABLES.partitionWithDupCheck(partitionName).getAbsolutePath(), TABLES_REF);

    LOGNF(PLUGIN, logPath, INFO) << "flashing " << imageName << " to " << partitionName << std::endl;

    if (FLAGS.onLogical && !TABLES.isLogical(partitionName)) {
      if (FLAGS.forceProcess)
        LOGNF(PLUGIN, logPath, WARNING) << "Partition " << partitionName << " is exists but not logical. Ignoring (from --force, -f)."
                                        << std::endl;
      else
        return PairError("Used --logical (-l) flag but is not logical partition: %s", partitionName.data());
    }

    LOGNF(PLUGIN, logPath, INFO) << "Using buffer size: " << buf << std::endl;

    // Automatically close file descriptors and delete allocated memories (arrays)
    Helper::garbageCollector collector;

    const int ffd = Helper::openAndAddToCloseList(imageName, collector, O_RDONLY);
    if (ffd < 0) return PairError("Can't open image file %s: %s", imageName.data(), strerror(errno));

    const int pfd = Helper::openAndAddToCloseList(TABLES.partitionWithDupCheck(partitionName, FLAGS.noWorkOnUsed).getAbsolutePath(),
                                                  collector, O_RDWR | O_TRUNC);
    if (pfd < 0) return PairError("Can't open partition: %s: %s", partitionName.data(), strerror(errno));

    LOGNF(PLUGIN, logPath, INFO) << "Writing image " << imageName << " to partition: " << partitionName << std::endl;
    auto *buffer = new (std::nothrow) char[buf];
    collector.delAfterProgress(buffer);
    memset(buffer, 0x00, buf);

    ssize_t bytesRead;
    while ((bytesRead = read(ffd, buffer, buf)) > 0) {
      if (const ssize_t bytesWritten = write(pfd, buffer, bytesRead); bytesWritten != bytesRead)
        return PairError("Can't write partition to output file %s: %s", imageName.data(), strerror(errno));
    }

    if (deleteAfterProgress) {
      LOGNF(PLUGIN, logPath, INFO) << "Deleting flash file: " << imageName << std::endl;
      if (!Helper::eraseEntry(imageName) && !FLAGS.quietProcess)
        WARNING(std::string("Cannot erase flash file: " + imageName + "\n").data());
    }

    return PairSuccess("%s is successfully wrote to %s partition", imageName.data(), partitionName.data());
  }

  bool run() override {
    processCommandLine(partitions, imageNames, rawPartitions, rawImageNames, ',', true);
    if (partitions.size() != imageNames.size())
      throw CLI::ValidationError("You must provide an image file(s) as long as the partition name(s)");

    for (size_t i = 0; i < partitions.size(); i++) {
      if (!imageDirectory.empty()) imageNames[i].insert(0, imageDirectory + '/');
    }

    std::vector<std::future<resultPair>> futures;
    for (size_t i = 0; i < partitions.size(); i++) {
      futures.push_back(std::async(std::launch::async, &FlashPlugin::runAsync, this, partitions[i], imageNames[i]));
      LOGNF(PLUGIN, logPath, INFO) << "Created thread for flashing image to " << partitions[i] << std::endl;
    }

    std::string end;
    bool endResult = true;
    for (auto &future : futures) {
      auto [fst, snd] = future.get();
      if (!snd) {
        end += fst + '\n';
        endResult = false;
      } else
        Out::println("%s", fst.c_str());
    }

    if (!endResult) throw Error("%s", end.c_str());

    LOGNF(PLUGIN, logPath, INFO) << "Operation successfully completed." << std::endl;
    return endResult;
  }

  std::string getName() override { return PLUGIN; }

  std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

#ifdef BUILTIN_PLUGINS
REGISTER_BUILTIN_PLUGIN(PartitionManager, FlashPlugin);
#else
extern "C" PartitionManager::BasicPlugin *create_plugin() { return new PartitionManager::FlashPlugin(); }
#endif // #ifdef BUILTIN_PLUGINS
