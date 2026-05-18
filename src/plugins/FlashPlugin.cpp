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

#include <future>
#include <fcntl.h>
#include <unistd.h>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#define PLUGIN "FlashPlugin"
#define PLUGIN_VERSION "1.2"

namespace PartitionManager {

class FlashPlugin final : public BasicPlugin {
  std::vector<std::string> partitions, imageNames;
  std::string rawPartitions, rawImageNames, imageDirectory;
  uint64_t bufferSize = 0;
  bool deleteAfterProgress = false;

  static constexpr uint64_t MIN_BUFFER_SIZE = 1024;                 ///< 1KB minimum buffer size
  static constexpr uint64_t MAX_BUFFER_SIZE = 128ULL * 1024 * 1024; ///< 128MB maximum buffer size

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;
  std::string logPath;

  PLUGIN_SECTION FlashPlugin() = default;
  PLUGIN_SECTION ~FlashPlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, const std::string &logpath, BasicFlags &mainFlags) override {
    logPath = logpath;
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    flags = &mainFlags;
    cmd = mainApp.addSubcommand("flash", "Flash image(s) to partition(s).");
    cmd->addOption("partition(s)", rawPartitions, "Partition name(s)")->required();
    cmd->addOption("imageFile(s)", rawImageNames, "Name(s) of image file(s)")->required();
    cmd->addOption("-b,--buffer-size", bufferSize, "Buffer size for reading image(s) and writing to partition(s)")
        ->transform(Helper::CMDLine::Transformers::AsSizeValue(false))
        ->defaultValue("1MB")
        ->check([](const std::string &input) -> std::string {
          try {
            uint64_t size = std::stoul(input);
            if (size < MIN_BUFFER_SIZE || size > MAX_BUFFER_SIZE) {
              return "Buffer size must be between 1KB and 128MB";
            }
            return "";
          } catch (...) {
            return "Invalid buffer size format";
          }
        });
    cmd->addOption("-I,--image-directory", imageDirectory, "Directory to find image(s) and flash to partition(s)");
    cmd->addFlag("-d,--delete", deleteAfterProgress, "Delete flash file(s) after progress.")->defaultValue(false);
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")->superior()->callback([this] {
      Out::println("{} v{}", getName(), getVersion());
      std::exit(0);
    });

    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  PLUGIN_SECTION AsyncResult_t runAsync(const std::string &partitionName, const std::string &imageName,
                                        PartitionMap::ProgressRenderer *renderer) const {
    if (!Helper::fileIsExists(imageName)) return AsyncResult_t::Error("Couldn't find image file: {}", imageName);
    if (!Tables.hasPartition(partitionName)) return AsyncResult_t::Error("Couldn't find partition: {}", partitionName);

    auto &partition = Tables.partitionWithDupCheck(partitionName, Flags.noWorkOnUsed)->get();
    if (partition.size() == 0) return AsyncResult_t::Error("Partition {} is empty", partitionName);

    const uint64_t imageSize = Helper::fileSize(imageName);
    if (imageSize == 0) return AsyncResult_t::Error("Image file {} is empty", imageName);

    const uint64_t buf = std::clamp<uint64_t>(bufferSize, MIN_BUFFER_SIZE, std::min<uint64_t>(bufferSize, partition.size()));

    if (imageSize > partition.size())
      return AsyncResult_t::Error("Image file {} ({} bytes) is larger than partition {} ({} bytes)", imageName, imageSize,
                                  partitionName, partition.size());

    LOGNF(PLUGIN, logPath, INFO) << "Flashing " << imageName << " to " << partitionName << std::endl;

    if (Flags.onLogical && !Tables.isLogical(partitionName)) {
      if (Flags.forceProcess)
        LOGNF(PLUGIN, logPath, WARNING) << "Partition " << partitionName << " exists but is not logical. Ignoring (from --force, -f)."
                                        << std::endl;
      else
        return AsyncResult_t::Error("Used --logical (-l) flag but partition is not logical: {}", partitionName);
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
      return AsyncResult_t::Error("Failed to write image {} to partition {}: {}", imageName, partitionName, ec.message());
    }
    if (progress) progress->finished.store(true, std::memory_order_relaxed);

    if (deleteAfterProgress) {
      LOGNF(PLUGIN, logPath, INFO) << "Deleting flash file: " << imageName << std::endl;
      if (!Helper::eraseEntry(imageName) && !Flags.quietProcess) {
        LOGNF(PLUGIN, logPath, WARNING) << "Cannot erase flash file: " << imageName << std::endl;
      }
    }

    return AsyncResult_t::Success("Image {} successfully flashed to partition {}", imageName, partitionName);
  }

  PLUGIN_SECTION bool run() override {
    processCommandLine(partitions, imageNames, rawPartitions, rawImageNames, ',', true);
    if (partitions.size() != imageNames.size())
      throw Error("You must provide an image file(s) as long as the partition name(s)").cmdlineError().withCode(EX_USAGE);

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
