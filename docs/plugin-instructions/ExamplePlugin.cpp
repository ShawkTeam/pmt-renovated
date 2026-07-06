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
 * @file ExamplePlugin.cpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Example plugin demonstrating basic plugin functionality.
 */

#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <nlohmann/json.hpp>

#define PLUGIN "ExamplePlugin"
#define PLUGIN_VERSION "1.1"

namespace PartitionManager {

/**
 * @brief Example plugin demonstrating basic plugin functionality.
 *
 * This plugin shows how to:
 * - Create a basic plugin with CLI integration
 * - Access partition information
 * - Perform simple operations on partitions
 * - Use async operations with progress reporting
 * - Output data in different formats
 */
class ExamplePlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  std::string rawPartitions;
  bool jsonFormat = false;
  bool detailed = false;
  bool countOnly = false;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  PLUGIN_SECTION ExamplePlugin() = default;
  PLUGIN_SECTION ~ExamplePlugin() override = default;

  /**
   * @brief Called when the plugin is loaded.
   * This is where you set up your CLI command and options.
   */
  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);

    flags = &mainFlags;
    // Create the subcommand for this plugin
    cmd = mainApp.addSubcommand("example", "Example plugin demonstrating partition operations");
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")
        ->superior()
        ->callback(Helper::CMDLine::Callbacks::ViewPluginVersion(PLUGIN, PLUGIN_VERSION));

    // Add command line options
    cmd->addOption("partition(s)", rawPartitions, "Partition name(s) to analyze")->delimiter(',');
    cmd->addFlag("-j,--json", jsonFormat, "Output results in JSON format")->defaultValue(false);
    cmd->addFlag("-d,--detailed", detailed, "Show detailed information")->defaultValue(false);
    cmd->addFlag("-c,--count", countOnly, "Only show partition count")->defaultValue(false);

    return true;
  }

  /**
   * @brief Called when the plugin is unloaded.
   * Clean up any resources here.
   */
  PLUGIN_SECTION bool onUnload() override {
    Log::info("{}::onUnload() trigger. Bye!", PLUGIN);
    cmd = nullptr;
    return true;
  }

  /**
   * @brief Check if this plugin was used in the command line.
   */
  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  /**
   * @brief Main plugin execution logic.
   * This is where your plugin's main functionality goes.
   */
  PLUGIN_SECTION bool run() override {
    // Process command line arguments
    if (!rawPartitions.empty()) partitions = Helper::CMDLine::split(rawPartitions, ',');

    if (countOnly)
      return showPartitionCount();
    else if (partitions.empty())
      return showAllPartitions();
    else
      return analyzeSpecificPartitions();
  }

  /**
   * @brief Get plugin name.
   */
  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  /**
   * @brief Get plugin version.
   */
  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }

private:
  /**
   * @brief Show total partition count.
   */
  bool showPartitionCount() {
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();
    auto pParts = pTab->partitions();
    auto dParts = dTab->partitions();

    if (jsonFormat) {
      nlohmann::json j;
      j["total_partitions"] = pParts.size() + dParts.size();
      j["logical_partitions"] = dParts.size();
      j["physical_partitions"] = pParts.size();
      Log::println("{}", j.dump(2));
    } else {
      Log::println("Total partitions: {}", pParts.size() + dParts.size());
      Log::println("Logical partitions: {}", dParts.size());
      Log::println("Physical partitions: {}", pParts.size());
    }

    return true;
  }

  /**
   * @brief Show information about all partitions.
   */
  bool showAllPartitions() {
    if (jsonFormat)
      return showAllPartitionsJson();
    else
      return showAllPartitionsText();
  }

  /**
   * @brief Show all partitions in text format.
   */
  bool showAllPartitionsText() {
    Log::println("=== All Partitions ===");

    auto getter = [this](const PartitionMap::Partition_t &partition) -> bool {
      if (detailed)
        Log::println("Name: {} | Table: {} | Size: {} | Logical: {} | Path: {}", partition.name(),
                     partition.isLogicalPartition() ? "N/A" : partition.tableName(),
                     partition.formattedSizeString(PartitionMap::MiB, true), partition.isLogicalPartition(),
                     partition.absolutePath().string());
      else
        Log::println("partition={} table={} size={} isLogical={}", partition.name(),
                     partition.isLogicalPartition() ? "" : partition.tableName(),
                     partition.formattedSizeString(PartitionMap::MiB, true), partition.isLogicalPartition());
      return true;
    };

    Tables.forEach(getter);
    return true;
  }

  /**
   * @brief Show all partitions in JSON format.
   */
  bool showAllPartitionsJson() {
    nlohmann::json j;
    j["partitions"] = nlohmann::json::array();

    auto getter = [&j](const PartitionMap::Partition_t &partition) -> bool {
      nlohmann::json part;
      part["name"] = partition.name();
      part["table"] = partition.isLogicalPartition() ? "" : partition.tableName();
      part["size_bytes"] = partition.size();
      part["size_human"] = partition.formattedSizeString(PartitionMap::MiB, true);
      part["is_logical"] = partition.isLogicalPartition();
      part["path"] = partition.absolutePath().string();
      j["partitions"].push_back(part);
      return true;
    };

    Tables.forEach(getter);
    Log::println("{}", j.dump(2));
    return true;
  }

  /**
   * @brief Analyze specific partitions.
   */
  bool analyzeSpecificPartitions() {
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();

    // Validate that all requested partitions exist
    for (const auto &partition : partitions) {
      if (!pTab->hasPartition(partition) && !dTab->hasPartition(partition))
        throw PluginError("Couldn't find partition: {}", partition);
    }

    if (jsonFormat)
      return analyzeSpecificPartitionsJson();
    else
      return analyzeSpecificPartitionsText();
  }

  /**
   * @brief Analyze specific partitions in text format.
   */
  bool analyzeSpecificPartitionsText() {
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();
    Log::println("=== Partition Analysis ===");

    for (const auto &partitionName : partitions) {
      if (pTab->hasPartition(partitionName)) {
        const auto &partition = pTab->partitionWithDupCheck(partitionName)->get();
        analyzePartitionText(partition);
      } else if (dTab->hasPartition(partitionName)) {
        const auto &partition = dTab->partition(partitionName)->get();
        analyzePartitionText(partition);
      }
    }

    return true;
  }

  /**
   * @brief Analyze a single partition in text format.
   */
  void analyzePartitionText(const PartitionMap::Partition_t &partition) {
    Log::println("\n--- Partition: {} ---", partition.name());
    Log::println("Type: {}", partition.isLogicalPartition() ? "Logical" : "Physical");
    if (!partition.isLogicalPartition()) Log::println("Table: {}", partition.tableName());
    Log::println("Size: {} ({})", partition.formattedSizeString(PartitionMap::MiB, true), partition.size());
    Log::println("Path: {}", partition.absolutePath().string());

    if (detailed) {
      // Additional detailed information
      Log::println("Device: {}", partition.absolutePath().parent_path().string());
      Log::println("Readable: {}", std::filesystem::exists(partition.absolutePath()));
    }
  }

  /**
   * @brief Analyze specific partitions in JSON format.
   */
  bool analyzeSpecificPartitionsJson() {
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();
    nlohmann::json j;
    j["analysis"] = nlohmann::json::array();

    for (const auto &partitionName : partitions) {
      if (pTab->hasPartition(partitionName)) {
        const auto &partition = pTab->partitionWithDupCheck(partitionName)->get();
        j["analysis"].push_back(analyzePartitionJson(partition));
      } else if (dTab->hasPartition(partitionName)) {
        const auto &partition = dTab->partition(partitionName)->get();
        j["analysis"].push_back(analyzePartitionJson(partition));
      }
    }

    Log::println("{}", j.dump(2));
    return true;
  }

  /**
   * @brief Analyze a single partition in JSON format.
   */
  nlohmann::json analyzePartitionJson(const PartitionMap::Partition_t &partition) {
    nlohmann::json part;
    part["name"] = partition.name();
    part["type"] = partition.isLogicalPartition() ? "logical" : "physical";

    if (!partition.isLogicalPartition()) part["table"] = partition.tableName();

    part["size"] = {{"bytes", partition.size()}, {"human_readable", partition.formattedSizeString(PartitionMap::MiB, true)}};

    part["path"] = partition.absolutePath().string();
    part["device"] = partition.absolutePath().parent_path().string();
    part["exists"] = std::filesystem::exists(partition.absolutePath());

    if (detailed) part["additional_info"] = "Detailed analysis would go here";

    return part;
  }
};

} // namespace PartitionManager

// Register the plugin with the plugin system
REGISTER_PLUGIN(PartitionManager, ExamplePlugin)
