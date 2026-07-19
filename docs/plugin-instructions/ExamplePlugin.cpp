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
 * @brief Example plugin demonstrating basic plugin functionality using RapidJSON.
 */

#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#define PLUGIN "ExamplePlugin"
#define PLUGIN_VERSION "1.1"

namespace PartitionManager {

/**
 * @brief Example plugin demonstrating basic plugin functionality.
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

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);

    flags = &mainFlags;
    cmd = mainApp.addSubcommand("example", "Example plugin demonstrating partition operations");
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")
        ->superior()
        ->callback(Helper::CMDLine::Callbacks::ViewPluginVersion(PLUGIN, PLUGIN_VERSION));

    cmd->addOption("partition(s)", rawPartitions, "Partition name(s) to analyze")->delimiter(',');
    cmd->addFlag("-j,--json", jsonFormat, "Output results in JSON format")->defaultValue(false);
    cmd->addFlag("-d,--detailed", detailed, "Show detailed information")->defaultValue(false);
    cmd->addFlag("-c,--count", countOnly, "Only show partition count")->defaultValue(false);

    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    Log::info("{}::onUnload() trigger. Bye!", PLUGIN);
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  PLUGIN_SECTION bool run() override {
    if (!rawPartitions.empty()) partitions = Helper::CMDLine::split(rawPartitions, ',');

    if (countOnly)
      return showPartitionCount();
    else if (partitions.empty())
      return showAllPartitions();
    else
      return analyzeSpecificPartitions();
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }
  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }

private:
  bool showPartitionCount() {
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();
    auto pParts = pTab->partitions();
    auto dParts = dTab->partitions();

    if (jsonFormat) {
      rapidjson::Document doc;
      doc.SetObject();
      auto &alc = doc.GetAllocator();
      doc.AddMember("total_partitions", (uint64_t)(pParts.size() + dParts.size()), alc);
      doc.AddMember("logical_partitions", (uint64_t)dParts.size(), alc);
      doc.AddMember("physical_partitions", (uint64_t)pParts.size(), alc);

      rapidjson::StringBuffer buffer;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      Log::println("{}", buffer.GetString());
    } else {
      Log::println("Total partitions: {}", pParts.size() + dParts.size());
      Log::println("Logical partitions: {}", dParts.size());
      Log::println("Physical partitions: {}", pParts.size());
    }

    return true;
  }

  bool showAllPartitions() { return jsonFormat ? showAllPartitionsJson() : showAllPartitionsText(); }

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

  bool showAllPartitionsJson() {
    rapidjson::Document doc;
    doc.SetObject();
    auto &alc = doc.GetAllocator();
    rapidjson::Value arr(rapidjson::kArrayType);

    Tables.forEach([&](const PartitionMap::Partition_t &partition) -> bool {
      rapidjson::Value part(rapidjson::kObjectType);
      part.AddMember("name", rapidjson::Value(partition.name().c_str(), alc), alc);
      part.AddMember("table", rapidjson::Value(partition.isLogicalPartition() ? "" : partition.tableName().c_str(), alc), alc);
      part.AddMember("size_bytes", partition.size(), alc);
      part.AddMember("size_human", rapidjson::Value(partition.formattedSizeString(PartitionMap::MiB, true).c_str(), alc), alc);
      part.AddMember("is_logical", partition.isLogicalPartition(), alc);
      part.AddMember("path", rapidjson::Value(partition.absolutePath().string().c_str(), alc), alc);
      arr.PushBack(part, alc);
      return true;
    });

    doc.AddMember("partitions", arr, alc);
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    Log::println("{}", buffer.GetString());
    return true;
  }

  bool analyzeSpecificPartitions() {
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();

    for (const auto &partition : partitions) {
      if (!pTab->hasPartition(partition) && !dTab->hasPartition(partition))
        throw PluginError("Couldn't find partition: {}", partition);
    }

    return jsonFormat ? analyzeSpecificPartitionsJson() : analyzeSpecificPartitionsText();
  }

  bool analyzeSpecificPartitionsText() {
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();
    Log::println("=== Partition Analysis ===");
    for (const auto &name : partitions) {
      if (pTab->hasPartition(name))
        analyzePartitionText(pTab->partitionWithDupCheck(name)->get());
      else if (dTab->hasPartition(name))
        analyzePartitionText(dTab->partition(name)->get());
    }
    return true;
  }

  void analyzePartitionText(const PartitionMap::Partition_t &partition) {
    Log::println("\n--- Partition: {} ---", partition.name());
    Log::println("Type: {}", partition.isLogicalPartition() ? "Logical" : "Physical");
    if (!partition.isLogicalPartition()) Log::println("Table: {}", partition.tableName());
    Log::println("Size: {} ({})", partition.formattedSizeString(PartitionMap::MiB, true), partition.size());
    Log::println("Path: {}", partition.absolutePath().string());
    if (detailed) {
      Log::println("Device: {}", partition.absolutePath().parent_path().string());
      Log::println("Readable: {}", std::filesystem::exists(partition.absolutePath()));
    }
  }

  bool analyzeSpecificPartitionsJson() {
    auto pTab = Flags.partitionTables.first.get();
    auto dTab = Flags.partitionTables.second.get();
    rapidjson::Document doc;
    doc.SetObject();
    auto &alc = doc.GetAllocator();
    rapidjson::Value arr(rapidjson::kArrayType);

    for (const auto &name : partitions) {
      if (pTab->hasPartition(name))
        arr.PushBack(analyzePartitionJson(pTab->partitionWithDupCheck(name)->get(), alc), alc);
      else if (dTab->hasPartition(name))
        arr.PushBack(analyzePartitionJson(dTab->partition(name)->get(), alc), alc);
    }
    doc.AddMember("analysis", arr, alc);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    Log::println("{}", buffer.GetString());
    return true;
  }

  rapidjson::Value analyzePartitionJson(const PartitionMap::Partition_t &partition, rapidjson::Document::AllocatorType &alc) {
    rapidjson::Value part(rapidjson::kObjectType);
    part.AddMember("name", rapidjson::Value(partition.name().c_str(), alc), alc);
    part.AddMember("type", rapidjson::Value(partition.isLogicalPartition() ? "logical" : "physical", alc), alc);
    if (!partition.isLogicalPartition()) part.AddMember("table", rapidjson::Value(partition.tableName().c_str(), alc), alc);

    rapidjson::Value sizeObj(rapidjson::kObjectType);
    sizeObj.AddMember("bytes", partition.size(), alc);
    sizeObj.AddMember("human_readable", rapidjson::Value(partition.formattedSizeString(PartitionMap::MiB, true).c_str(), alc), alc);
    part.AddMember("size", sizeObj, alc);

    part.AddMember("path", rapidjson::Value(partition.absolutePath().string().c_str(), alc), alc);
    part.AddMember("device", rapidjson::Value(partition.absolutePath().parent_path().string().c_str(), alc), alc);
    part.AddMember("exists", std::filesystem::exists(partition.absolutePath()), alc);

    if (detailed) part.AddMember("additional_info", "Detailed analysis would go here", alc);
    return part;
  }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, ExamplePlugin)