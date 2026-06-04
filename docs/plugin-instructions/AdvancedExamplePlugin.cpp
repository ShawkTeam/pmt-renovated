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
 * @file AdvancedExamplePlugin.cpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Advanced example plugin demonstrating complex plugin functionality.
 */

#include <chrono>
#include <future>
#include <map>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <nlohmann/json.hpp>

#define PLUGIN "AdvancedExamplePlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

/**
 * @brief Advanced example plugin demonstrating complex plugin functionality.
 *
 * This plugin shows how to:
 * - Use advanced CLI options with validation
 * - Perform complex partition analysis
 * - Use different output formats
 * - Handle errors gracefully
 * - Use advanced partition operations
 */
class AdvancedExamplePlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  std::string rawPartitions, outputFile;
  std::string sizeUnit = "MiB";
  std::string sortBy = "name";
  bool jsonFormat = false;
  bool includeTables = false;
  bool onlyLogical = false;
  bool onlyPhysical = false;
  bool reverseSort = false;
  uint64_t minSize = 0;
  uint64_t maxSize = UINT64_MAX;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;
  std::string logPath;

  PLUGIN_SECTION AdvancedExamplePlugin() = default;
  PLUGIN_SECTION ~AdvancedExamplePlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, const std::string &logpath, BasicFlags &mainFlags) override {
    logPath = logpath;
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;

    flags = &mainFlags;
    cmd = mainApp.addSubcommand("advanced", "Advanced example plugin with complex operations");

    // Partition selection options
    auto *partitionGroup = cmd->addOptionGroup("Partition Selection", "Select which partitions to analyze");
    cmd->addOption("partition(s)", rawPartitions, "Specific partition name(s) to analyze", partitionGroup)->delimiter(',');
    cmd->addFlag("-l,--logical", onlyLogical, "Analyze only logical partitions", partitionGroup);
    cmd->addFlag("-p,--physical", onlyPhysical, "Analyze only physical partitions", partitionGroup);

    // Size filtering options
    auto *sizeGroup = cmd->addOptionGroup("Size Filtering", "Filter partitions by size");
    cmd->addOption("--min-size", minSize, "Minimum partition size (bytes)", sizeGroup)
        ->transform(Helper::CMDLine::Transformers::AsSizeValue());
    cmd->addOption("--max-size", maxSize, "Maximum partition size (bytes)", sizeGroup)
        ->transform(Helper::CMDLine::Transformers::AsSizeValue());

    // Output options
    auto *outputGroup = cmd->addOptionGroup("Output Options", "Control output format and content");
    cmd->addFlag("-j,--json", jsonFormat, "Output in JSON format", outputGroup);
    cmd->addFlag("-t,--tables", includeTables, "Include table information", outputGroup);
    cmd->addOption("-o,--output", outputFile, "Output to file instead of stdout", outputGroup);

    // Sorting options
    auto *sortGroup = cmd->addOptionGroup("Sorting", "Control partition sorting");
    cmd->addOption("--sort-by", sortBy, "Sort by: name, size, type, table", sortGroup)
        ->check(Helper::CMDLine::Checkers::IsMember({"name", "size", "type", "table"}));
    cmd->addFlag("-r,--reverse", reverseSort, "Reverse sort order", sortGroup);

    // Size unit options
    auto *unitGroup = cmd->addOptionGroup("Size Units", "Control size display units");
    cmd->addOption("--unit", sizeUnit, "Size unit: B, KiB, MiB, GiB", unitGroup)
        ->check(Helper::CMDLine::Checkers::IsMember({"B", "KiB", "MiB", "GiB"}));

    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  PLUGIN_SECTION bool run() override {
    // Validate conflicting options
    if (onlyLogical && onlyPhysical) throw Helper::Error("Cannot specify both --logical and --physical");

    // Parse partitions if specified
    if (!rawPartitions.empty()) partitions = Helper::CMDLine::split(rawPartitions, ',');

    // Get partition list
    std::vector<PartitionMap::Partition_t> partitionList = getPartitionList();

    // Apply filters
    partitionList = applyFilters(partitionList);

    // Sort partitions
    partitionList = sortPartitions(partitionList);

    // Generate output
    if (jsonFormat) {
      generateJsonOutput(partitionList);
    } else {
      generateTextOutput(partitionList);
    }

    return true;
  }

private:
  /**
   * @brief Get the list of partitions to analyze.
   */
  std::vector<PartitionMap::Partition_t> getPartitionList() {
    std::vector<PartitionMap::Partition_t> result;

    if (!partitions.empty()) {
      // Use specified partitions
      for (const auto &name : partitions) {
        if (Tables.hasPartition(name)) {
          result.push_back(Tables.partitionWithDupCheck(name)->get());
        } else if (Tables.hasLogicalPartition(name)) {
          result.push_back(Tables.partition(name)->get());
        } else {
          throw PluginError("Partition not found: {}", name);
        }
      }
    } else if (onlyLogical) {
      // Use only logical partitions
      auto logicalParts = Tables.logicalPartitions();
      for (const auto &part : logicalParts) {
        result.push_back(part.get());
      }
    } else if (onlyPhysical) {
      // Use only physical partitions
      auto physicalParts = Tables.partitions();
      for (const auto &part : physicalParts) {
        result.push_back(part.get());
      }
    } else {
      // Use all partitions
      auto allParts = Tables.allPartitions();
      for (const auto &part : allParts) {
        result.push_back(part.get());
      }
    }

    return result;
  }

  /**
   * @brief Apply size filters to partition list.
   */
  std::vector<PartitionMap::Partition_t> applyFilters(const std::vector<PartitionMap::Partition_t> &_partitions) {
    std::vector<PartitionMap::Partition_t> result;

    for (const auto &partition : _partitions) {
      uint64_t size = partition.size();

      if (size >= minSize && size <= maxSize) {
        result.push_back(partition);
      }
    }

    return result;
  }

  /**
   * @brief Sort partitions according to specified criteria.
   */
  std::vector<PartitionMap::Partition_t> sortPartitions(const std::vector<PartitionMap::Partition_t> &_partitions) {
    std::vector<PartitionMap::Partition_t> result = _partitions;

    std::sort(result.begin(), result.end(), [this](const PartitionMap::Partition_t &a, const PartitionMap::Partition_t &b) {
      bool comparison = false;

      if (sortBy == "name") {
        comparison = a.name() < b.name();
      } else if (sortBy == "size") {
        comparison = a.size() < b.size();
      } else if (sortBy == "type") {
        comparison = a.isLogicalPartition() < b.isLogicalPartition();
      } else if (sortBy == "table") {
        std::string tableA = a.isLogicalPartition() ? "" : a.tableName();
        std::string tableB = b.isLogicalPartition() ? "" : b.tableName();
        comparison = tableA < tableB;
      }

      return reverseSort ? !comparison : comparison;
    });

    return result;
  }

  /**
   * @brief Generate text output.
   */
  void generateTextOutput(const std::vector<PartitionMap::Partition_t> &_partitions) {
    PartitionMap::SizeUnit unit = parseSizeUnit();

    std::string output;
    output += "=== Advanced Partition Analysis ===\n";
    output += std::format("Total partitions: {}\n", _partitions.size());
    output += std::format("Size unit: {}\n", sizeUnit);
    output += std::format("Sort by: {} {}\n\n", sortBy, reverseSort ? "(descending)" : "(ascending)");

    for (const auto &partition : _partitions) {
      output += std::format("Name: {}\n", partition.name());
      output += std::format("  Type: {}\n", partition.isLogicalPartition() ? "Logical" : "Physical");

      if (!partition.isLogicalPartition() && includeTables) {
        output += std::format("  Table: {}\n", partition.tableName());
      }

      output += std::format("  Size: {} ({})\n", partition.formattedSizeString(unit, true), partition.size());
      output += std::format("  Path: {}\n", partition.absolutePath().string());
      output += "\n";
    }

    if (outputFile.empty()) {
      Out::println("{}", output);
    } else {
      std::ofstream file(outputFile);
      if (file) {
        file << output;
        Out::println("Output written to: {}", outputFile);
      } else {
        throw PluginError("Failed to write to file: {}", outputFile);
      }
    }
  }

  /**
   * @brief Generate JSON output.
   */
  void generateJsonOutput(const std::vector<PartitionMap::Partition_t> &_partitions) {
    PartitionMap::SizeUnit unit = parseSizeUnit();

    nlohmann::json j;
    j["analysis_info"] = {{"total_partitions", _partitions.size()},
                          {"size_unit", sizeUnit},
                          {"sort_by", sortBy},
                          {"reverse_sort", reverseSort},
                          {"min_size_filter", minSize},
                          {"max_size_filter", maxSize == UINT64_MAX ? "unlimited" : std::to_string(maxSize)}};

    j["partitions"] = nlohmann::json::array();

    for (const auto &partition : _partitions) {
      nlohmann::json part;
      part["name"] = partition.name();
      part["type"] = partition.isLogicalPartition() ? "logical" : "physical";
      part["size"] = {{"bytes", partition.size()}, {"formatted", partition.formattedSizeString(unit, true)}};
      part["path"] = partition.absolutePath().string();

      if (!partition.isLogicalPartition() && includeTables) {
        part["table"] = partition.tableName();
      }

      j["partitions"].push_back(part);
    }

    std::string output = j.dump(2);

    if (outputFile.empty()) {
      Out::println("{}", output);
    } else {
      std::ofstream file(outputFile);
      if (file) {
        file << output;
        Out::println("JSON output written to: {}", outputFile);
      } else {
        throw PluginError("Failed to write to file: {}", outputFile);
      }
    }
  }

  /**
   * @brief Parse size unit string to enum.
   */
  PartitionMap::SizeUnit parseSizeUnit() {
    if (sizeUnit == "B") return PartitionMap::BYTE;
    if (sizeUnit == "KiB") return PartitionMap::KiB;
    if (sizeUnit == "MiB") return PartitionMap::MiB;
    if (sizeUnit == "GiB") return PartitionMap::GiB;
    return PartitionMap::MiB; // default
  }

public:
  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, AdvancedExamplePlugin)
