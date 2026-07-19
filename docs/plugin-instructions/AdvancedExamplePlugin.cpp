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
#include <algorithm>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#define PLUGIN "AdvancedExamplePlugin"
#define PLUGIN_VERSION "1.1"

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

  PLUGIN_SECTION AdvancedExamplePlugin() = default;
  PLUGIN_SECTION ~AdvancedExamplePlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    flags = &mainFlags;
    cmd = mainApp.addSubcommand("advanced", "Advanced example plugin with complex operations");
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")
        ->superior()
        ->callback(Helper::CMDLine::Callbacks::ViewPluginVersion(PLUGIN, PLUGIN_VERSION));

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
    Log::info("{}::onUnload() trigger. Bye!", PLUGIN);
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
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();

    if (!partitions.empty()) {
      // Use specified partitions
      for (const auto &name : partitions) {
        if (pTab->hasPartition(name)) {
          result.push_back(pTab->partitionWithDupCheck(name)->get());
        } else if (dTab->hasPartition(name)) {
          result.push_back(dTab->partition(name)->get());
        } else {
          throw PluginError("Partition not found: {}", name);
        }
      }
    } else if (onlyLogical) {
      // Use only logical partitions
      auto logicalParts = dTab->partitions();
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
      auto pParts = pTab->partitions();
      auto dParts = dTab->partitions();
      for (const auto &part : pParts)
        result.push_back(part.get());
      for (const auto &part : dParts)
        result.push_back(part.get());
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
      Log::println("{}", output);
    } else {
      std::ofstream file(outputFile);
      if (file) {
        file << output;
        Log::println("Output written to: {}", outputFile);
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
    rapidjson::Document doc;
    doc.SetObject();
    auto &alc = doc.GetAllocator();

    // analysis_info object
    rapidjson::Value info(rapidjson::kObjectType);
    info.AddMember("total_partitions", (uint64_t)_partitions.size(), alc);
    info.AddMember("size_unit", rapidjson::Value(sizeUnit.c_str(), alc), alc);
    info.AddMember("sort_by", rapidjson::Value(sortBy.c_str(), alc), alc);
    info.AddMember("reverse_sort", reverseSort, alc);
    info.AddMember("min_size_filter", minSize, alc);

    if (maxSize == UINT64_MAX)
      info.AddMember("max_size_filter", "unlimited", alc);
    else
      info.AddMember("max_size_filter", rapidjson::Value(std::to_string(maxSize).c_str(), alc), alc);

    doc.AddMember("analysis_info", info, alc);

    // partitions array
    rapidjson::Value arr(rapidjson::kArrayType);
    for (const auto &partition : _partitions) {
      rapidjson::Value part(rapidjson::kObjectType);
      part.AddMember("name", rapidjson::Value(partition.name().c_str(), alc), alc);
      part.AddMember("type", rapidjson::Value(partition.isLogicalPartition() ? "logical" : "physical", alc), alc);

      // nested size object
      rapidjson::Value sz(rapidjson::kObjectType);
      sz.AddMember("bytes", partition.size(), alc);
      sz.AddMember("formatted", rapidjson::Value(partition.formattedSizeString(unit, true).c_str(), alc), alc);
      part.AddMember("size", sz, alc);

      part.AddMember("path", rapidjson::Value(partition.absolutePath().string().c_str(), alc), alc);

      if (!partition.isLogicalPartition() && includeTables)
        part.AddMember("table", rapidjson::Value(partition.tableName().c_str(), alc), alc);
      arr.PushBack(part, alc);
    }
    doc.AddMember("partitions", arr, alc);

    // Write output
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    if (outputFile.empty())
      Log::println("{}", buffer.GetString());
    else {
      std::ofstream file(outputFile);
      if (file) {
        file << buffer.GetString();
        Log::println("JSON output written to: {}", outputFile);
      } else
        throw PluginError("Failed to write to file: {}", outputFile);
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
