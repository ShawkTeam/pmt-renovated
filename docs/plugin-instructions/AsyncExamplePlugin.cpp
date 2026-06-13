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
 * @file AsyncExamplePlugin.cpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Async example plugin demonstrating asynchronous operations.
 */

#include <chrono>
#include <future>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#define PLUGIN "AsyncExamplePlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

/**
 * @brief Async example plugin demonstrating asynchronous operations.
 *
 * This plugin shows how to:
 * - Create plugins with async operations
 * - Use AsyncManager for parallel processing
 * - Implement progress reporting
 * - Handle async results
 */
class AsyncExamplePlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  std::string rawPartitions;
  uint64_t delayMs = 1000;
  bool simulateWork = false;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;
  std::string logPath;

  PLUGIN_SECTION AsyncExamplePlugin() = default;
  PLUGIN_SECTION ~AsyncExamplePlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, const std::string &logpath, BasicFlags &mainFlags) override {
    logPath = logpath;
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;

    flags = &mainFlags;
    cmd = mainApp.addSubcommand("async-example", "Example plugin demonstrating async operations");

    cmd->addOption("partition(s)", rawPartitions, "Partition name(s) to process asynchronously")->delimiter(',');
    cmd->addOption("--delay", delayMs, "Delay in milliseconds for simulated work")->defaultValue(1000);
    cmd->addFlag("--simulate", simulateWork, "Simulate async work with delay")->defaultValue(false);

    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  /**
   * @brief Async worker function that processes a single partition.
   */
  PLUGIN_SECTION AsyncResult_t processPartitionAsync(const std::string &partitionName) const {
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();
    LOGNF(PLUGIN, logPath, INFO) << "Processing partition: " << partitionName << std::endl;

    // Check if partition exists
    if (!pTab->hasPartition(partitionName) && !dTab->hasPartition(partitionName)) {
      return AsyncResult_t::Error("Partition not found: {}", partitionName);
    }

    // Simulate work if requested
    if (simulateWork) {
      std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }

    // Get partition information
    if (pTab->hasPartition(partitionName)) {
      const auto &partition = pTab->partitionWithDupCheck(partitionName)->get();
      return AsyncResult_t::Success("Processed partition: {} (size: {})", partitionName,
                                    partition.formattedSizeString(PartitionMap::MiB, true));
    } else {
      const auto &partition = dTab->partition(partitionName)->get();
      return AsyncResult_t::Success("Processed logical partition: {} (size: {})", partitionName,
                                    partition.formattedSizeString(PartitionMap::MiB, true));
    }
  }

  PLUGIN_SECTION bool run() override {
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();

    if (rawPartitions.empty()) {
      // If no partitions specified, process all partitions
      auto pParts = pTab->partitions();
      auto dParts = dTab->partitions();
      for (const auto &part : pParts)
        partitions.push_back(part.get().name());
      for (const auto &part : dParts)
        partitions.push_back(part.get().name());
    } else
      partitions = Helper::CMDLine::split(rawPartitions, ',');

    if (partitions.empty()) {
      Out::println("No partitions to process.");
      return true;
    }

    // Create async manager
    Helper::AsyncManager<AsyncResult_t> manager;
    manager.print = false;

    // Add async tasks
    for (const auto &partition : partitions) {
      manager.addProcess(&AsyncExamplePlugin::processPartitionAsync, this, partition);
      LOGNF(PLUGIN, logPath, INFO) << "Added async task for partition: " << partition << std::endl;
    }

    // Start all async operations
    manager.startAll();

    // Get results
    auto results = manager.getResults();

    // Process results
    int successCount = 0;
    int errorCount = 0;

    for (const auto &result : results) {
      if (result.isSuccess()) {
        successCount++;
        Out::println("✓ {}", result.getMessage());
      } else {
        errorCount++;
        Out::println("✗ {}", result.getMessage());
      }
    }

    Out::println("\nAsync processing completed:");
    Out::println("Successful: {}", successCount);
    Out::println("Failed: {}", errorCount);
    Out::println("Total: {}", successCount + errorCount);

    return manager.finalize();
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, AsyncExamplePlugin)
