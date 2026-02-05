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
