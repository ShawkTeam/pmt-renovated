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

#define PLUGIN "CleanLogPlugin"
#define PLUGIN_VERSION "1.1"

namespace PartitionManager {

class CleanLogPlugin final : public BasicPlugin {
public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;
  std::string logPath;

  PLUGIN_SECTION CleanLogPlugin() = default;
  PLUGIN_SECTION ~CleanLogPlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, const std::string &logpath, BasicFlags &mainFlags) override {
    logPath = logpath;
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.addSubcommand("clean-logs", "Clean PMT logs.");
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")
        ->superior()
        ->callback(Helper::CMDLine::Callbacks::ViewPluginVersion(PLUGIN, PLUGIN_VERSION));
    flags = &mainFlags;
    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  PLUGIN_SECTION bool run() override {
    LOGNF(PLUGIN, logPath, INFO) << "Removing log file: " << Flags.logFile << std::endl;
    Helper::Logger::Properties::setLogging(true); // eraseEntry writes log!
    return Helper::eraseEntry(Flags.logFile);
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, CleanLogPlugin)
