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

#define PLUGIN "RealPathPlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class RealPathPlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  bool byName = false;

public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;

  ~RealPathPlugin() override = default;

  bool onLoad(CLI::App &mainApp, FlagsBase &mainFlags) override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.add_subcommand("real-path", "Tell real paths of partition(s)");
    flags = mainFlags;
    cmd->add_option("partition(s)", partitions, "Partition name(s)")->required()->delimiter(',');
    cmd->add_flag("--by-name", byName, "Print by-name path(s)")->default_val(false);

    return true;
  }

  bool onUnload() override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    return true;
  }

  bool used() override { return cmd->parsed(); }

  bool run() override {
    for (const auto &partition : partitions) {
      if (!TABLES.hasPartition(partition)) throw Error("Couldn't find partition: %s", partition.data());

      auto &part = TABLES.partitionWithDupCheck(partition, FLAGS.noWorkOnUsed);
      if (FLAGS.onLogical && !part.isLogicalPartition()) {
        if (FLAGS.forceProcess)
          LOGN(PLUGIN, WARNING) << "Partition " << partition << " is exists but not logical. Ignoring (from --force, -f)."
                                << std::endl;
        else
          throw Error("Used --logical (-l) flag but is not logical partition: %s", partition.data());
      }

      if (byName)
        Out::println("%s", part.getPathByName().c_str());
      else
        Out::println("%s", part.getAbsolutePath().c_str());
    }

    return true;
  }

  std::string getName() override { return PLUGIN; }

  std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

#ifdef BUILTIN_PLUGINS
REGISTER_BUILTIN_PLUGIN(PartitionManager, RealPathPlugin);
#else
extern "C" PartitionManager::BasicPlugin *create_plugin() { return new PartitionManager::RealPathPlugin(); }
#endif // #ifdef BUILTIN_PLUGINS
