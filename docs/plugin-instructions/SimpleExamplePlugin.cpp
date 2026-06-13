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
#define PLUGIN_VERSION "1.0"

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
  std::string logPath;

  PLUGIN_SECTION SimpleExamplePlugin() = default;
  PLUGIN_SECTION ~SimpleExamplePlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, const std::string &logpath, BasicFlags &mainFlags) override {
    logPath = logpath;
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;

    flags = &mainFlags;
    // Create a simple subcommand
    cmd = mainApp.addSubcommand("simple", "Simple example plugin");

    // Add a single option for partition name
    cmd->addOption("partition", partitionName, "Partition name to examine")->required();

    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  PLUGIN_SECTION bool run() override {
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();

    // Check if partition exists
    if (!pTab->hasPartition(partitionName) && !dTab->hasPartition(partitionName)) {
      Out::println("Error: Partition '{}' not found!", partitionName);
      return false;
    }

    // Get partition information
    if (pTab->hasPartition(partitionName)) {
      const auto &partition = pTab->partitionWithDupCheck(partitionName)->get();

      Out::println("=== Partition Information ===");
      Out::println("Name: {}", partition.name());
      Out::println("Type: {}", partition.isLogicalPartition() ? "Logical" : "Physical");

      if (!partition.isLogicalPartition()) Out::println("Table: {}", partition.tableName());

      Out::println("Size: {}", partition.formattedSizeString(PartitionMap::MiB, true));
      Out::println("Path: {}", partition.absolutePath().string());

    } else {
      const auto &partition = dTab->partition(partitionName)->get();

      Out::println("=== Logical Partition Information ===");
      Out::println("Name: {}", partition.name());
      Out::println("Size: {}", partition.formattedSizeString(PartitionMap::MiB, true));
      Out::println("Path: {}", partition.absolutePath().string());
    }

    return true;
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, SimpleExamplePlugin)
