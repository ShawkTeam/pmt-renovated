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

#include <fcntl.h>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <nlohmann/json.hpp>

#define PLUGIN "InfoPlugin"
#define PLUGIN_VERSION "1.1"

namespace PartitionManager {

class InfoPlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  std::string jNamePartition, jNameTable, jNameSize, jNameLogical;
  int jIndentSize = 2;
  bool jsonFormat = false, asByte = true, asKiloBytes = false, asMega = false, asGiga = false;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;
  std::string logPath;

  PLUGIN_SECTION InfoPlugin() = default;
  PLUGIN_SECTION ~InfoPlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, const std::string &logpath, BasicFlags &mainFlags) override {
    logPath = logpath;
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.addSubcommand("info", "Tell info(s) of input partition list.")
              ->footer("Use get-all or getvar-all as partition name for getting "
                       "info's of all partitions.\nUse get-logicals as partition "
                       "name for getting info's of logical partitions.\n"
                       "Use get-physical as partition name for getting info's of "
                       "physical partitions.");
    flags = &mainFlags;
    cmd->addOption("partition(s)", partitions, "Partition name(s).")->required();
    cmd->addFlag("-J,--json", jsonFormat,
                 "Print info(s) as JSON body. The body of each partition will "
                 "be written separately")
        ->defaultValue(false);
    cmd->addFlag("--as-byte", asByte, "View sizes as byte.")->defaultValue(true);
    cmd->addFlag("--as-kilobyte", asKiloBytes, "View sizes as kilobyte.")->defaultValue(false);
    cmd->addFlag("--as-megabyte", asMega, "View sizes as megabyte.")->defaultValue(false);
    cmd->addFlag("--as-gigabyte", asGiga, "View sizes as gigabyte.")->defaultValue(false);
    cmd->addOption("--json-partition-name", jNamePartition, "Specify partition name element for JSON body")->defaultValue("name");
    cmd->addOption("--json-table-name", jNameTable, "Specify table elemtn name for JSON body")->defaultValue("table");
    cmd->addOption("--json-size-name", jNameSize, "Specify size element name for JSON body")->defaultValue("size");
    cmd->addOption("--json-logical-name", jNameLogical, "Specify logical element name for JSON body")->defaultValue("isLogical");
    cmd->addOption("--json-indent-size", jIndentSize, "Set JSON indent size for printing to screen")->defaultValue(2);
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")->superior()->callback([this] {
      Out::println("{} v{}", getName(), getVersion());
      std::exit(0);
    });

    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  PLUGIN_SECTION bool run() override {
    std::vector<PartitionMap::Partition_t> jParts;
    PartitionMap::SizeUnit multiple;
    if (asByte) multiple = PartitionMap::BYTE;
    if (asKiloBytes) multiple = PartitionMap::KiB;
    if (asMega) multiple = PartitionMap::MiB;
    if (asGiga) multiple = PartitionMap::GiB;

    auto getter = [this, &jParts, &multiple] FOREACH_PARTITIONS_LAMBDA_PARAMETERS_CONST -> bool {
      if (jsonFormat)
        jParts.push_back(partition);
      else
        Out::println("partition={} table={} size={} isLogical={}", partition.name(),
                     partition.isLogicalPartition() ? "" : partition.tableName(), partition.formattedSizeString(multiple, true),
                     partition.isLogicalPartition());

      return true;
    };

    if (partitions.back() == "get-all" || partitions.back() == "getvar-all")
      Tables.forEach(getter);
    else if (partitions.back() == "get-logicals")
      Tables.forEachLogicalPartitions(getter);
    else if (partitions.back() == "get-physicals")
      Tables.forEachPartitions(getter);
    else {
      for (const auto &partition : partitions) {
        if (!Tables.hasPartition(partition)) throw Error("Couldn't find partition: {}", partition);
      }
      Tables.forEachFor(partitions, getter);
    }

    if (jsonFormat) {
      nlohmann::json j;
      j["multipleType"] = PartitionMap::Extra::getSizeUnitAsString(multiple);
      j["partitions"] = nlohmann::json::array();
      for (auto &part : jParts)
        j["partitions"].push_back({{jNamePartition, part.name()},
                                   {jNameTable, part.isLogicalPartition() ? "" : part.tableName()},
                                   {jNameSize, std::stoull(part.formattedSizeString(multiple, true))},
                                   {jNameLogical, part.isLogicalPartition()}});

      Out::println("{}", j.dump(jIndentSize));
    }

    return true;
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, InfoPlugin)
