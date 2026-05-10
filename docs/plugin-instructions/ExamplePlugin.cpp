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
#include <CLI11.hpp>
#include <nlohmann/json.hpp>

#define PLUGIN "ExamplePlugin"
#define PLUGIN_VERSION "1.0"

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
  CLI::App *cmd = nullptr;
  BasicFlags *flags = nullptr;
  std::string logPath;

  PLUGIN_SECTION ExamplePlugin() = default;
  PLUGIN_SECTION ~ExamplePlugin() override = default;

  /**
   * @brief Called when the plugin is loaded.
   * This is where you set up your CLI command and options.
   */
  PLUGIN_SECTION bool onLoad(CLI::App &mainApp, const std::string &logpath, BasicFlags &mainFlags) override {
    logPath = logpath;
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;

    flags = &mainFlags;
    // Create the subcommand for this plugin
    cmd = mainApp.add_subcommand("example", "Example plugin demonstrating partition operations");
    cmd->fallthrough();

    // Add command line options
    cmd->add_option("partition(s)", rawPartitions, "Partition name(s) to analyze")->delimiter(',');

    cmd->add_flag("-j,--json", jsonFormat, "Output results in JSON format")->default_val(false);

    cmd->add_flag("-d,--detailed", detailed, "Show detailed information")->default_val(false);

    cmd->add_flag("-c,--count", countOnly, "Only show partition count")->default_val(false);

    return true;
  }

  /**
   * @brief Called when the plugin is unloaded.
   * Clean up any resources here.
   */
  PLUGIN_SECTION bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  /**
   * @brief Check if this plugin was used in the command line.
   */
  PLUGIN_SECTION bool used() override { return cmd->parsed(); }

  /**
   * @brief Main plugin execution logic.
   * This is where your plugin's main functionality goes.
   */
  PLUGIN_SECTION bool run() override {
    // Process command line arguments
    if (!rawPartitions.empty()) {
      partitions = CLI::detail::split(rawPartitions, ',');
    }

    if (countOnly) {
      return showPartitionCount();
    } else if (partitions.empty()) {
      return showAllPartitions();
    } else {
      return analyzeSpecificPartitions();
    }
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
    auto allParts = Tables.allPartitions();
    auto logicalParts = Tables.logicalPartitions();

    if (jsonFormat) {
      nlohmann::json j;
      j["total_partitions"] = allParts.size();
      j["logical_partitions"] = logicalParts.size();
      j["physical_partitions"] = allParts.size() - logicalParts.size();
      Out::println("{}", j.dump(2));
    } else {
      Out::println("Total partitions: {}", allParts.size());
      Out::println("Logical partitions: {}", logicalParts.size());
      Out::println("Physical partitions: {}", allParts.size() - logicalParts.size());
    }

    return true;
  }

  /**
   * @brief Show information about all partitions.
   */
  bool showAllPartitions() {
    if (jsonFormat) {
      return showAllPartitionsJson();
    } else {
      return showAllPartitionsText();
    }
  }

  /**
   * @brief Show all partitions in text format.
   */
  bool showAllPartitionsText() {
    Out::println("=== All Partitions ===");

    auto getter = [this](const PartitionMap::Partition_t &partition) -> bool {
      if (detailed) {
        Out::println("Name: {} | Table: {} | Size: {} | Logical: {} | Path: {}", partition.name(),
                     partition.isLogicalPartition() ? "N/A" : partition.tableName(),
                     partition.formattedSizeString(PartitionMap::MiB, true), partition.isLogicalPartition(),
                     partition.absolutePath().string());
      } else {
        Out::println("partition={} table={} size={} isLogical={}", partition.name(),
                     partition.isLogicalPartition() ? "" : partition.tableName(),
                     partition.formattedSizeString(PartitionMap::MiB, true), partition.isLogicalPartition());
      }
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
    Out::println("{}", j.dump(2));
    return true;
  }

  /**
   * @brief Analyze specific partitions.
   */
  bool analyzeSpecificPartitions() {
    // Validate that all requested partitions exist
    for (const auto &partition : partitions) {
      if (!Tables.hasPartition(partition) && !Tables.hasLogicalPartition(partition)) {
        throw PluginError("Couldn't find partition: {}", partition);
      }
    }

    if (jsonFormat) {
      return analyzeSpecificPartitionsJson();
    } else {
      return analyzeSpecificPartitionsText();
    }
  }

  /**
   * @brief Analyze specific partitions in text format.
   */
  bool analyzeSpecificPartitionsText() {
    Out::println("=== Partition Analysis ===");

    for (const auto &partitionName : partitions) {
      if (Tables.hasPartition(partitionName)) {
        const auto &partition = Tables.partitionWithDupCheck(partitionName)->get();
        analyzePartitionText(partition);
      } else if (Tables.hasLogicalPartition(partitionName)) {
        const auto &partition = Tables.partition(partitionName)->get();
        analyzePartitionText(partition);
      }
    }

    return true;
  }

  /**
   * @brief Analyze a single partition in text format.
   */
  void analyzePartitionText(const PartitionMap::Partition_t &partition) {
    Out::println("\n--- Partition: {} ---", partition.name());
    Out::println("Type: {}", partition.isLogicalPartition() ? "Logical" : "Physical");
    if (!partition.isLogicalPartition()) {
      Out::println("Table: {}", partition.tableName());
    }
    Out::println("Size: {} ({})", partition.formattedSizeString(PartitionMap::MiB, true), partition.size());
    Out::println("Path: {}", partition.absolutePath().string());

    if (detailed) {
      // Additional detailed information
      Out::println("Device: {}", partition.absolutePath().parent_path().string());
      Out::println("Readable: {}", std::filesystem::exists(partition.absolutePath()));
    }
  }

  /**
   * @brief Analyze specific partitions in JSON format.
   */
  bool analyzeSpecificPartitionsJson() {
    nlohmann::json j;
    j["analysis"] = nlohmann::json::array();

    for (const auto &partitionName : partitions) {
      if (Tables.hasPartition(partitionName)) {
        const auto &partition = Tables.partitionWithDupCheck(partitionName)->get();
        j["analysis"].push_back(analyzePartitionJson(partition));
      } else if (Tables.hasLogicalPartition(partitionName)) {
        const auto &partition = Tables.partition(partitionName)->get();
        j["analysis"].push_back(analyzePartitionJson(partition));
      }
    }

    Out::println("{}", j.dump(2));
    return true;
  }

  /**
   * @brief Analyze a single partition in JSON format.
   */
  nlohmann::json analyzePartitionJson(const PartitionMap::Partition_t &partition) {
    nlohmann::json part;
    part["name"] = partition.name();
    part["type"] = partition.isLogicalPartition() ? "logical" : "physical";

    if (!partition.isLogicalPartition()) {
      part["table"] = partition.tableName();
    }

    part["size"] = {{"bytes", partition.size()}, {"human_readable", partition.formattedSizeString(PartitionMap::MiB, true)}};

    part["path"] = partition.absolutePath().string();
    part["device"] = partition.absolutePath().parent_path().string();
    part["exists"] = std::filesystem::exists(partition.absolutePath());

    if (detailed) {
      part["additional_info"] = "Detailed analysis would go here";
    }

    return part;
  }
};

} // namespace PartitionManager

// Register the plugin with the plugin system
REGISTER_PLUGIN(PartitionManager, ExamplePlugin)
