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

#define PLUGIN "PartitionSizePlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class PartitionSizePlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  bool onlySize = false, asByte = false, asKiloBytes = false, asMega = false, asGiga = false;

public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;

  ~PartitionSizePlugin() override = default;

  bool onLoad(CLI::App &mainApp, FlagsBase &mainFlags) override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.add_subcommand("sizeof", "Tell size(s) of input partition list")
              ->footer("Use get-all or getvar-all as partition name for getting "
                       "sizes of all partitions.\nUse get-logicals as partition "
                       "name for getting sizes of logical partitions.\n"
                       "Use get-physical as partition name for getting sizes of "
                       "physical partitions.");
    flags = mainFlags;
    cmd->add_option("partition(s)", partitions, "Partition name(s).")->required()->delimiter(',');
    cmd->add_flag("--as-byte", asByte, "Tell input size of partition list as byte.")->default_val(false);
    cmd->add_flag("--as-kilobyte", asKiloBytes, "Tell input size of partition list as kilobyte.")->default_val(false);
    cmd->add_flag("--as-megabyte", asMega, "Tell input size of partition list as megabyte.")->default_val(true);
    cmd->add_flag("--as-gigabyte", asGiga, "Tell input size of partition list as gigabyte.")->default_val(false);
    cmd->add_flag("--only-size", onlySize,
                  "Tell input size of partition list as not printing multiple "
                  "and partition name.")
        ->default_val(false);
    return true;
  }

  bool onUnload() override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    return true;
  }

  bool used() override { return cmd->parsed(); }

  bool run() override {
    PartitionMap::SizeUnit multiple = {};
    if (asByte) multiple = PartitionMap::BYTE;
    if (asKiloBytes) multiple = PartitionMap::KiB;
    if (asMega) multiple = PartitionMap::MiB;
    if (asGiga) multiple = PartitionMap::GiB;

    auto getter = [this, &multiple] FOREACH_PARTITIONS_LAMBDA_PARAMETERS -> bool {
      if (onlySize)
        Out::println("%s", partition.getFormattedSizeString(multiple, true).c_str());
      else
        Out::println("%s: %s", partition.getName().c_str(), partition.getFormattedSizeString(multiple).c_str());

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

    return true;
  }

  std::string getName() override { return PLUGIN; }

  std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

#ifdef BUILTIN_PLUGINS
REGISTER_BUILTIN_PLUGIN(PartitionManager, PartitionSizePlugin);
#else
extern "C" PartitionManager::BasicPlugin *create_plugin() { return new PartitionManager::PartitionSizePlugin(); }
#endif // #ifdef BUILTIN_PLUGINS
