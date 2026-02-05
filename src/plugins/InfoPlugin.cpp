/*
Copyright 2025 Yağız Zengin

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <nlohmann/json.hpp>

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

  ~InfoPlugin() override = default;

  bool onLoad(CLI::App &mainApp, FlagsBase &mainFlags) override {
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
            {{jNamePartition, part.getName()}, {jNameSize, part.getSize()}, {jNameLogical, part.isLogicalPartition()}});

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
