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
 * @file ErasePlugin.cpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Implementation of the ErasePlugin for erasing partitions.
 *
 * This file implements the ErasePlugin class which provides functionality
 * to securely erase partitions by writing zeros to the entire partition.
 */

#include <cerrno>
#include <future>
#include <fcntl.h>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#define PLUGIN "ErasePlugin"
#define PLUGIN_VERSION "1.3"

namespace PartitionManager {

/**
 * @brief Plugin for erasing partitions by writing zeros.
 *
 * This plugin provides functionality to securely erase partitions by writing
 * zeros to the entire partition. It supports configurable buffer sizes and
 * can process multiple partitions asynchronously.
 */
class ErasePlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  uint64_t bufferSize = 0;

  static constexpr uint64_t MIN_BUFFER_SIZE = 1024;                 ///< 1KB minimum buffer size
  static constexpr uint64_t MAX_BUFFER_SIZE = 128ULL * 1024 * 1024; ///< 128MB maximum buffer size
  static constexpr uint64_t DEFAULT_BUFFER_SIZE = 4ULL * 1024;      ///< 4KB default buffer size

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  /// @brief Default constructor.
  PLUGIN_SECTION ErasePlugin() = default;
  /// @brief Default destructor.
  PLUGIN_SECTION ~ErasePlugin() override = default;

  /**
   * @brief Load the plugin and register its subcommand.
   *
   * @param mainApp The main application instance.
   * @param mainFlags The global flags structure.
   * @return true if the plugin loaded successfully.
   */
  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    cmd = mainApp.addSubcommand("erase", "Writes zero bytes to partition(s).");
    flags = &mainFlags;
    cmd->addOption("partition(s)", partitions, "Partition name(s)")->required();
    cmd->addOption("-b,--buffer-size", bufferSize, "Buffer size for writing zero bytes to partition(s)")
        ->transform(Helper::CMDLine::Transformers::AsSizeValue(false))
        ->defaultValue("4KB")
        ->check(Helper::CMDLine::Checkers::BufferSizeCheck(MIN_BUFFER_SIZE, MAX_BUFFER_SIZE));
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
   * @brief Run the erase operation asynchronously for a single partition.
   *
   * @param partitionName The name of the partition to erase.
   * @return AsyncResult_t Result of the asynchronous operation.
   */
  PLUGIN_SECTION AsyncResult_t runAsync(const std::string &partitionName) const {
    std::optional<PartitionMap::TableType> tType;
    auto *table = getCorrectTableObj(partitionName, Flags.partitionTables.first.get(), Flags.partitionTables.second.get(), tType);
    const PartitionMap::Partition_t *partition = setupPartition(partitionName, table);

    if (!partition) return AsyncResult_t::Error("Couldn't find partition: {}", partitionName);
    if (partition->size() == 0) return AsyncResult_t::Error("Partition {} is empty", partitionName);

    if (Flags.onLogical && tType != PartitionMap::DYNAMIC) {
      if (Flags.forceProcess)
        Log::warning("Partition {} exists but is not logical. Ignoring (from --force, -f).", partitionName);
      else
        return AsyncResult_t::Error("Used --logical (-l) flag but partition is not logical: {}", partitionName);
    }

    uint64_t buf = std::clamp<uint64_t>(bufferSize, MIN_BUFFER_SIZE, std::min<uint64_t>(bufferSize, partition->size()));
    setupBufferSize(buf, partitionName, table);
    Log::info("Using buffer size: {}", buf);

    auto pfd = Helper::UniqueFD(partition->absolutePath(), O_WRONLY);
    if (!pfd) return AsyncResult_t::Error("Can't open partition {}: {}", partitionName, strerror(errno));

    if (!Flags.forceProcess) {
      if (!Helper::confirmPropt("Are you sure you want to continue? This could render your device "
                                "unusable! Do not continue if you "
                                "do not know what you are doing!")) {
        throw Error("Operation canceled by user.");
      }
    }

    Log::info("Writing zero bytes to partition: {}", partitionName);
    auto buffer = std::make_unique<char[]>(buf);
    std::memset(buffer.get(), 0, buf);

    ssize_t bytesWritten = 0;
    const uint64_t partitionSize = partition->size();

    while (bytesWritten < partitionSize) {
      size_t toWrite = std::min<uint64_t>(buf, partitionSize - bytesWritten);

      if (const ssize_t result = pfd.write(buffer.get(), toWrite); result == -1) {
        return AsyncResult_t::Error("Can't write zero bytes to partition {}: {}", partitionName, strerror(errno));
      } else if (result == 0) {
        return AsyncResult_t::Error("Write operation returned 0 bytes for partition {}", partitionName);
      } else {
        bytesWritten += result;
      }
    }

    return AsyncResult_t::Success("Successfully wrote zero bytes to partition {}", partitionName);
  }

  /**
   * @brief Run the erase operation for all specified partitions.
   *
   * @return true if all erase operations succeeded.
   */
  PLUGIN_SECTION bool run() override {
    Helper::AsyncManager<AsyncResult_t> manager;
    for (const auto &partitionName : partitions) {
      manager.addProcess(&ErasePlugin::runAsync, this, partitionName);
      Log::info("Created thread for erasing partition: {}", partitionName);
    }

    manager.startAll();
    return manager();
  }

  /// @brief Get the plugin name.
  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  /// @brief Get the plugin version.
  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, ErasePlugin)
