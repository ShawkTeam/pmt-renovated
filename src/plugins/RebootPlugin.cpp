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
#define PLUGIN_VERSION "1.1"

namespace PartitionManager {

class RebootPlugin final : public BasicPlugin {
  std::string rebootTarget;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;
  std::string logPath;

  PLUGIN_SECTION RebootPlugin() = default;
  PLUGIN_SECTION ~RebootPlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, const std::string &logpath, BasicFlags &mainFlags) override {
    logPath = logpath;
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.addSubcommand("reboot", "Reboot the device.");
    flags = &mainFlags;
    cmd->addOption("rebootTarget", rebootTarget, "Reboot target (default: normal)");
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")->superior()->callback([this] {
      Out::println("{} v{}", getName(), getVersion());
      std::exit(0);
    });
    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  PLUGIN_SECTION bool run() override {
    LOGNF(PLUGIN, logPath, INFO) << "Rebooting device!!! (custom reboot target: " << (rebootTarget.empty() ? "none" : rebootTarget)
                                 << std::endl;

    if (Helper::Android::reboot(rebootTarget))
      Out::println("Reboot command was sent");
    else
      throw Error("Cannot reboot device!");

    return true;
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, RebootPlugin)
