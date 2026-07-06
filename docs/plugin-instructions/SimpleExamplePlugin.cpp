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
 * @file SimpleExamplePlugin.cpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Simple example plugin demonstrating minimal plugin structure.
 */

#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#define PLUGIN "SimpleExamplePlugin"
#define PLUGIN_VERSION "1.1"

namespace PartitionManager {

/**
 * @brief Simple example plugin demonstrating minimal plugin structure.
 *
 * This is the most basic plugin example showing:
 * - Minimal plugin structure
 * - Basic CLI integration
 * - Simple partition access
 * - Basic output formatting
 */
class SimpleExamplePlugin final : public BasicPlugin {
  std::string partitionName;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  PLUGIN_SECTION SimpleExamplePlugin() = default;
  PLUGIN_SECTION ~SimpleExamplePlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);

    flags = &mainFlags;
    // Create a simple subcommand
    cmd = mainApp.addSubcommand("simple", "Simple example plugin");

    // Add a single option for partition name
    cmd->addOption("partition", partitionName, "Partition name to examine")->required();

    // Add version flag
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")
        ->superior()
        ->callback(Helper::CMDLine::Callbacks::ViewPluginVersion(PLUGIN, PLUGIN_VERSION));

    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    Log::info("{}::onUnload() trigger. Bye!", PLUGIN);
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  PLUGIN_SECTION bool run() override {
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();

    // Check if partition exists
    if (!pTab->hasPartition(partitionName) && !dTab->hasPartition(partitionName)) {
      Log::println("Error: Partition '{}' not found!", partitionName);
      return false;
    }

    // Get partition information
    if (pTab->hasPartition(partitionName)) {
      const auto &partition = pTab->partitionWithDupCheck(partitionName)->get();

      Log::println("=== Partition Information ===");
      Log::println("Name: {}", partition.name());
      Log::println("Type: {}", partition.isLogicalPartition() ? "Logical" : "Physical");

      if (!partition.isLogicalPartition()) Log::println("Table: {}", partition.tableName());

      Log::println("Size: {}", partition.formattedSizeString(PartitionMap::MiB, true));
      Log::println("Path: {}", partition.absolutePath().string());

    } else {
      const auto &partition = dTab->partition(partitionName)->get();

      Log::println("=== Logical Partition Information ===");
      Log::println("Name: {}", partition.name());
      Log::println("Size: {}", partition.formattedSizeString(PartitionMap::MiB, true));
      Log::println("Path: {}", partition.absolutePath().string());
    }

    return true;
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, SimpleExamplePlugin)
