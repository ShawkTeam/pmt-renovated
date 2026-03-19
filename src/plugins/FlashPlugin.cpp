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
  BasicFlags *flags;
  const char *logPath = nullptr;

  PLUGIN_SECTION FlashPlugin() DEFAULT_PLUGIN_CONSTRUCTOR;
  PLUGIN_SECTION ~FlashPlugin() override = default;

  PLUGIN_SECTION bool onLoad(CLI::App &mainApp, const std::string &logpath, BasicFlags &mainFlags) override {
    logPath = logpath.c_str();
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    flags = &mainFlags;
    cmd = mainApp.add_subcommand("flash", "Flash image(s) to partition(s)");
    cmd->fallthrough();
    cmd->add_option("partition(s)", rawPartitions, "Partition name(s)")->required();
    cmd->add_option("imageFile(s)", rawImageNames, "Name(s) of image file(s)")->required();
    cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for reading image(s) and writing to partition(s)")
        ->transform(CLI::AsSizeValue(false))
        ->default_val("1MB");
    cmd->add_option("-I,--image-directory", imageDirectory, "Directory to find image(s) and flash to partition(s)");
    cmd->add_flag("-d,--delete", deleteAfterProgress, "Delete flash file(s) after progress.")->default_val(false);

    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->parsed(); }

  PLUGIN_SECTION AsyncResult_t runAsync(const std::string &partitionName, const std::string &imageName,
                                        PartitionMap::ProgressRenderer *renderer) const {
    if (!Helper::fileIsExists(imageName)) return AsyncResult_t::Error("Couldn't find image file: {}", imageName);
    if (!Tables.hasPartition(partitionName)) return AsyncResult_t::Error("Couldn't find partition: {}", partitionName);

    auto &partition = Tables.partitionWithDupCheck(partitionName, Flags.noWorkOnUsed)->get();
    const uint64_t buf = std::min<uint64_t>(bufferSize, partition.size());

    if (Helper::fileSize(imageName) > partition.size())
      return AsyncResult_t::Error("{} is larger than {} partition size!", imageName, partitionName);

    LOGNF(PLUGIN, logPath, INFO) << "flashing " << imageName << " to " << partitionName << std::endl;

    if (Flags.onLogical && !Tables.isLogical(partitionName)) {
      if (Flags.forceProcess)
        LOGNF(PLUGIN, logPath, WARNING) << "Partition " << partitionName << " is exists but not logical. Ignoring (from --force, -f)."
                                        << std::endl;
      else
        return AsyncResult_t::Error("Used --logical (-l) flag but is not logical partition: {}", partitionName);
    }

    LOGNF(PLUGIN, logPath, INFO) << "Using buffer size: " << buf << std::endl;

    std::shared_ptr<PartitionMap::Progress_t> progress;
    if (renderer) progress = renderer->add(partitionName, partition.size());

    std::error_code ec;
    PartitionMap::Partition_t::IOCallback cb = nullptr;
    if (progress) {
      cb = [&progress](uint64_t done, uint64_t) { progress->done.store(done, std::memory_order_relaxed); };
    }

    if (!partition.write(ec, imageName, buf, cb)) {
      if (progress) progress->failed.store(true, std::memory_order_relaxed);
      return AsyncResult_t::Error("Failed to write {} image to {} partition: {}", imageName, partitionName, ec.message());
    }
    if (progress) progress->finished.store(true, std::memory_order_relaxed);

    if (deleteAfterProgress) {
      LOGNF(PLUGIN, logPath, INFO) << "Deleting flash file: " << imageName << std::endl;
      if (!Helper::eraseEntry(imageName) && !Flags.quietProcess)
        WARNING(std::string("Cannot erase flash file: " + imageName + "\n").data());
    }

    return AsyncResult_t::Success("{} is successfully wrote to {} partition", imageName, partitionName);
  }

  PLUGIN_SECTION bool run() override {
    processCommandLine(partitions, imageNames, rawPartitions, rawImageNames, ',', true);
    if (partitions.size() != imageNames.size())
      throw CLI::ValidationError("You must provide an image file(s) as long as the partition name(s)");

    for (size_t i = 0; i < partitions.size(); i++) {
      if (!imageDirectory.empty()) imageNames[i].insert(0, imageDirectory + '/');
    }

    Helper::AsyncManager<AsyncResult_t> manager;
    manager.print = false;
    std::unique_ptr<PartitionMap::ProgressRenderer> renderer;
    if (!Flags.quietProcess) renderer = std::make_unique<PartitionMap::ProgressRenderer>();

    for (size_t i = 0; i < partitions.size(); i++) {
      manager.addProcess(&FlashPlugin::runAsync, this, partitions[i], imageNames[i], renderer.get());
      LOGNF(PLUGIN, logPath, INFO) << "Created thread for flashing image to " << partitions[i] << std::endl;
    }

    PLUGIN_END_WITH_RENDERER(renderer, manager);
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, FlashPlugin)
