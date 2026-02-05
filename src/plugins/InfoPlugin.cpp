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

#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <nlohmann/json.hpp>
#include <CLI11.hpp>

#define PLUGIN "InfoPlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class InfoPlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  std::string jNamePartition, jNameSize, jNameLogical;
  int jIndentSize = 2;
  bool jsonFormat = false, asByte = true, asKiloBytes = false, asMega = false, asGiga = false;

public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;
  const char* logPath = nullptr;

  ~InfoPlugin() override = default;

  bool onLoad(CLI::App &mainApp, const std::string& logpath, FlagsBase &mainFlags) override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.add_subcommand("info", "Tell info(s) of input partition list")
              ->footer("Use get-all or getvar-all as partition name for getting "
                       "info's of all partitions.\nUse get-logicals as partition "
                       "name for getting info's of logical partitions.\n"
                       "Use get-physical as partition name for getting info's of "
                       "physical partitions.");
    flags = mainFlags;
    cmd->add_option("partition(s)", partitions, "Partition name(s).")->required()->delimiter(',');
    cmd->add_flag("-J,--json", jsonFormat,
                  "Print info(s) as JSON body. The body of each partition will "
                  "be written separately")
        ->default_val(false);
    cmd->add_flag("--as-byte", asByte, "View sizes as byte.")->default_val(true);
    cmd->add_flag("--as-kilobyte", asKiloBytes, "View sizes as kilobyte.")->default_val(false);
    cmd->add_flag("--as-megabyte", asMega, "View sizes as megabyte.")->default_val(false);
    cmd->add_flag("--as-gigabyte", asGiga, "View sizes as gigabyte.")->default_val(false);
    cmd->add_option("--json-partition-name", jNamePartition, "Specify partition name element for JSON body")->default_val("name");
    cmd->add_option("--json-size-name", jNameSize, "Specify size element name for JSON body")->default_val("size");
    cmd->add_option("--json-logical-name", jNameLogical, "Specify logical element name for JSON body")->default_val("isLogical");
    cmd->add_option("--json-indent-size", jIndentSize, "Set JSON indent size for printing to screen")->default_val(2);

    return true;
  }

  bool onUnload() override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  bool used() override { return cmd->parsed(); }

  bool run() override {
    std::vector<PartitionMap::Partition_t> jParts;
    PartitionMap::SizeUnit multiple;
    if (asByte) multiple = PartitionMap::BYTE;
    if (asKiloBytes) multiple = PartitionMap::KiB;
    if (asMega) multiple = PartitionMap::MiB;
    if (asGiga) multiple = PartitionMap::GiB;

    auto getter = [this, &jParts, &multiple] FOREACH_PARTITIONS_LAMBDA_PARAMETERS -> bool {
      if (jsonFormat)
        jParts.push_back(partition);
      else
        Out::println("partition=%s size=%s isLogical=%s", partition.getName().c_str(),
                     partition.getFormattedSizeString(multiple, true).c_str(), partition.isLogicalPartition() ? "true" : "false");

      return true;
    };

    if (partitions.back() == "get-all" || partitions.back() == "getvar-all")
      TABLES.foreach (getter);
    else if (partitions.back() == "get-logicals")
      TABLES.foreachLogicalPartitions(getter);
    else if (partitions.back() == "get-physicals")
      TABLES.foreachPartitions(getter);
    else {
      for (const auto &partition : partitions) {
        if (!TABLES.hasPartition(partition)) throw Error("Couldn't find partition: %s", partition.c_str());
      }
      TABLES.foreachFor(partitions, getter);
    }

    if (jsonFormat) {
      nlohmann::json j;
      j["multipleType"] = PartitionMap::Extra::getSizeUnitAsString(multiple);
      j["partitions"] = nlohmann::json::array();
      for (auto &part : jParts)
        j["partitions"].push_back(
            {{jNamePartition, part.getName()}, {jNameSize, std::stoull(part.getFormattedSizeString(multiple, true))}, {jNameLogical, part.isLogicalPartition()}});

      Out::println("%s", j.dump(jIndentSize).data());
    }

    return true;
  }

  std::string getName() override { return PLUGIN; }

  std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

#ifdef BUILTIN_PLUGINS
REGISTER_BUILTIN_PLUGIN(PartitionManager, InfoPlugin);
#else
extern "C" PartitionManager::BasicPlugin *create_plugin() { return new PartitionManager::InfoPlugin(); }
#endif // #ifdef BUILTIN_PLUGINS
