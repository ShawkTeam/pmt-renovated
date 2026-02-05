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

#define PLUGIN "RebootPlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class RebootPlugin final : public BasicPlugin {
  std::string rebootTarget;

public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;

  ~RebootPlugin() override = default;

  bool onLoad(CLI::App &mainApp, FlagsBase &mainFlags) override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.add_subcommand("reboot", "Reboots device");
    flags = mainFlags;
    cmd->add_option("rebootTarget", rebootTarget, "Reboot target (default: normal)");
    return true;
  }

  bool onUnload() override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    return true;
  }

  bool used() override { return cmd->parsed(); }

  bool run() override {
    LOGN(PLUGIN, INFO) << "Rebooting device!!! (custom reboot target: " << (rebootTarget.empty() ? "none" : rebootTarget) << std::endl;

    if (Helper::androidReboot(rebootTarget))
      Out::println("Reboot command was sent");
    else
      throw Error("Cannot reboot device");

    return true;
  }

  std::string getName() override { return PLUGIN; }

  std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

#ifdef BUILTIN_PLUGINS
REGISTER_BUILTIN_PLUGIN(PartitionManager, RebootPlugin);
#else
extern "C" PartitionManager::BasicPlugin *create_plugin() { return new PartitionManager::RebootPlugin(); }
#endif // #ifdef BUILTIN_PLUGINS
