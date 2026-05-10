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
#include <CLI11.hpp>

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
  CLI::App *cmd = nullptr;
  BasicFlags *flags;
  const char *logPath = nullptr;

  PLUGIN_SECTION SimpleExamplePlugin() DEFAULT_PLUGIN_CONSTRUCTOR;
  PLUGIN_SECTION ~SimpleExamplePlugin() override = default;

  PLUGIN_SECTION bool onLoad(CLI::App &mainApp, const std::string &logpath, BasicFlags &mainFlags) override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;

    flags = &mainFlags;
    logPath = logpath.c_str();

    // Create a simple subcommand
    cmd = mainApp.add_subcommand("simple", "Simple example plugin");
    cmd->fallthrough();

    // Add a single option for partition name
    cmd->add_option("partition", partitionName, "Partition name to examine")->required();

    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->parsed(); }

  PLUGIN_SECTION bool run() override {
    // Check if partition exists
    if (!Tables.hasPartition(partitionName) && !Tables.hasLogicalPartition(partitionName)) {
      Out::println("Error: Partition '{}' not found!", partitionName);
      return false;
    }

    // Get partition information
    if (Tables.hasPartition(partitionName)) {
      const auto &partition = Tables.partitionWithDupCheck(partitionName)->get();

      Out::println("=== Partition Information ===");
      Out::println("Name: {}", partition.name());
      Out::println("Type: {}", partition.isLogicalPartition() ? "Logical" : "Physical");

      if (!partition.isLogicalPartition()) {
        Out::println("Table: {}", partition.tableName());
      }

      Out::println("Size: {}", partition.formattedSizeString(PartitionMap::MiB, true));
      Out::println("Path: {}", partition.absolutePath().string());

    } else {
      const auto &partition = Tables.partition(partitionName)->get();

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
