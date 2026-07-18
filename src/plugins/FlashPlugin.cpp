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
 * @file FlashPlugin.cpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Implementation of the FlashPlugin for flashing images to partitions.
 *
 * This file implements the FlashPlugin class which provides functionality
 * to write image files to partitions. It supports configurable buffer sizes
 * and can process multiple partitions asynchronously.
 */

#include <future>
#include <fcntl.h>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#define PLUGIN "FlashPlugin"
#define PLUGIN_VERSION "1.3"

namespace PartitionManager {

/**
 * @brief Plugin for flashing images to partitions.
 *
 * This plugin provides functionality to write image files to partitions.
 * It supports configurable buffer sizes, optional deletion of images after
 * flashing, and can process multiple partitions asynchronously.
 */
class FlashPlugin final : public BasicPlugin {
  std::vector<std::string> partitions, imageNames;
  std::string imageDirectory;
  uint64_t bufferSize = 0;
  bool deleteAfterProgress = false;

  static constexpr uint64_t MIN_BUFFER_SIZE = 1024;                 ///< 1KB minimum buffer size
  static constexpr uint64_t MAX_BUFFER_SIZE = 128ULL * 1024 * 1024; ///< 128MB maximum buffer size

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  /// @brief Default constructor.
  PLUGIN_SECTION FlashPlugin() = default;
  /// @brief Default destructor.
  PLUGIN_SECTION ~FlashPlugin() override = default;

  /**
   * @brief Load the plugin and register its subcommand.
   *
   * @param mainApp The main application instance.
   * @param mainFlags The global flags structure.
   * @return true if the plugin loaded successfully.
   */
  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    flags = &mainFlags;
    cmd = mainApp.addSubcommand("flash", "Flash image(s) to partition(s).");
    cmd->addOption("partition(s)", partitions, "Partition name(s)")->required();
    cmd->addOption("imageFile(s)", imageNames, "Name(s) of image file(s)")->required();
    cmd->addOption("-b,--buffer-size", bufferSize, "Buffer size for reading image(s) and writing to partition(s)")
        ->transform(Helper::CMDLine::Transformers::AsSizeValue(false))
        ->defaultValue("1MB")
        ->check(Helper::CMDLine::Checkers::BufferSizeCheck(MIN_BUFFER_SIZE, MAX_BUFFER_SIZE));
    cmd->addOption("-I,--image-directory", imageDirectory, "Directory to find image(s) and flash to partition(s)")
        ->check(Helper::CMDLine::Checkers::ExistingDirectory());
    cmd->addFlag("-d,--delete", deleteAfterProgress, "Delete flash file(s) after progress.")->defaultValue(false);
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")
        ->superior()
        ->callback(Helper::CMDLine::Callbacks::ViewPluginVersion(PLUGIN, PLUGIN_VERSION));

    return true;
  }

  /// @brief Unload the plugin and clean up resources.
  PLUGIN_SECTION bool onUnload() override {
    Log::info("{}::onUnload() trigger. Bye!", PLUGIN);
    cmd = nullptr;
    return true;
  }

  /// @brief Check if the plugin's subcommand was used.
  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  /**
   * @brief Run the flash operation asynchronously for a single partition.
   *
   * @param partitionName The name of the partition to flash.
   * @param imageName The path to the image file to flash.
   * @param renderer Optional progress renderer for displaying progress.
   * @return AsyncResult_t Result of the asynchronous operation.
   */
  PLUGIN_SECTION AsyncResult_t runAsync(const std::string &partitionName, const std::string &imageName,
                                        PartitionMap::ProgressRenderer *renderer) const {
    if (!Helper::fileIsExists(imageName)) return AsyncResult_t::Error("Couldn't find image file: {}", imageName);

    std::optional<PartitionMap::TableType> tType;
    auto *table = getCorrectTableObj(partitionName, Flags.partitionTables.first.get(), Flags.partitionTables.second.get(), tType);

    PartitionMap::Partition_t *partition = setupPartition(partitionName, table);
    if (!partition) return AsyncResult_t::Error("Couldn't find partition: {}", partitionName);
    if (partition->size() == 0) return AsyncResult_t::Error("Partition {} is empty", partitionName);

    const uint64_t imageSize = Helper::fileSize(imageName);
    if (imageSize == 0) return AsyncResult_t::Error("Image file {} is empty", imageName);

    const uint64_t buf = std::clamp<uint64_t>(bufferSize, MIN_BUFFER_SIZE, std::min<uint64_t>(bufferSize, partition->size()));

    if (imageSize > partition->size())
      return AsyncResult_t::Error("Image file {} ({} bytes) is larger than partition {} ({} bytes)", imageName, imageSize,
                                  partitionName, partition->size());

    Log::info("Flashing {} to {}", imageName, partitionName);
    if (Flags.onLogical && tType != PartitionMap::DYNAMIC) {
      if (Flags.forceProcess)
        Log::warning("Partition {} exists but is not logical. Ignoring (from --force, -f).", partitionName);
      else
        return AsyncResult_t::Error("Used --logical (-l) flag but partition is not logical: {}", partitionName);
    }
    Log::info("Using buffer size: {}", buf);

    std::shared_ptr<PartitionMap::Progress_t> progress;
    if (renderer) progress = renderer->add(partitionName, partition->size());

    PartitionMap::Partition_t::IOCallback cb = nullptr;
    if (progress) {
      cb = [&progress](uint64_t done, uint64_t) { progress->done.store(done, std::memory_order_relaxed); };
    }

    try {
      partition->write(imageName, buf, cb);
    } catch (Error &err) {
      if (progress) progress->failed.store(true, std::memory_order_relaxed);
      return AsyncResult_t::Error("Failed to write image {} to partition {}: {}", imageName, partitionName, err.what());
    }

    if (progress) progress->finished.store(true, std::memory_order_relaxed);

    if (deleteAfterProgress) {
      Log::info("Deleting flash file: {}", imageName);
      if (!Helper::eraseEntry(imageName) && !Flags.quietProcess) Log::warning("Cannot erase flash file: {}", imageName);
    }

    return AsyncResult_t::Success("Image {} successfully flashed to partition {}", imageName, partitionName);
  }

  /**
   * @brief Run the flash operation for all specified partitions.
   *
   * @return true if all flash operations succeeded.
   */
  PLUGIN_SECTION bool run() override {
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
      Log::info("Created thread for flashing image to {}", partitions[i]);
    }

    PLUGIN_END_WITH_RENDERER(renderer, manager);
  }

  /// @brief Get the plugin name.
  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  /// @brief Get the plugin version.
  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, FlashPlugin)
