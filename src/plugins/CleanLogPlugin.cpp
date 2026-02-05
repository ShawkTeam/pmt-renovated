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
#include <CLI11.hpp>

#define PLUGIN "CleanLogPlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class CleanLogPlugin final : public BasicPlugin {
public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;
  const char *logPath = nullptr;

  ~CleanLogPlugin() override = default;

  bool onLoad(CLI::App &mainApp, const std::string& logpath, FlagsBase &mainFlags) override {
    logPath = logpath.c_str();
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.add_subcommand("clean-logs", "Clean PMT logs.");
    flags = mainFlags;
    return true;
  }

  bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  bool used() override { return cmd->parsed(); }

  bool run() override {
    LOGNF(PLUGIN, logPath, INFO) << "Removing log file: " << FLAGS.logFile << std::endl;
    Helper::LoggingProperties::setLoggingState<YES>(); // eraseEntry writes log!
    return Helper::eraseEntry(FLAGS.logFile);
  }

  std::string getName() override { return PLUGIN; }

  std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

#ifdef BUILTIN_PLUGINS
REGISTER_BUILTIN_PLUGIN(PartitionManager, CleanLogPlugin);
#else
extern "C" PartitionManager::BasicPlugin *create_plugin() { return new PartitionManager::CleanLogPlugin(); }
#endif // #ifdef BUILTIN_PLUGINS
