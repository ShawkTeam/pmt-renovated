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

/**
 * @file RebootPlugin.cpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Implementation of the RebootPlugin for rebooting the device.
 *
 * This file implements the RebootPlugin class which provides functionality
 * to reboot the device with different targets (normal, recovery, bootloader, etc.).
 */

#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#define PLUGIN "RebootPlugin"
#define PLUGIN_VERSION "1.2"

namespace PartitionManager {

/**
 * @brief Plugin for rebooting the device.
 *
 * This plugin provides functionality to reboot the device with different
 * targets (normal, recovery, bootloader, etc.).
 */
class RebootPlugin final : public BasicPlugin {
  std::string rebootTarget;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  /// @brief Default constructor.
  PLUGIN_SECTION RebootPlugin() = default;
  /// @brief Default destructor.
  PLUGIN_SECTION ~RebootPlugin() override = default;

  /**
   * @brief Load the plugin and register its subcommand.
   *
   * @param mainApp The main application instance.
   * @param mainFlags The global flags structure.
   * @return true if the plugin loaded successfully.
   */
  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    cmd = mainApp.addSubcommand("reboot", "Reboot the device.");
    flags = &mainFlags;
    cmd->addOption("rebootTarget", rebootTarget, "Reboot target")->defaultValue("normal");
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")
        ->superior()
        ->callback(Helper::CMDLine::Callbacks::ViewPluginVersion(PLUGIN, PLUGIN_VERSION));
    return true;
  }

  /**
   * @brief Unload the plugin and clean up resources.
   *
   * @return true if the plugin unloaded successfully.
   */
  PLUGIN_SECTION bool onUnload() override {
    Log::info("{}::onUnload() trigger. Bye!", PLUGIN);
    cmd = nullptr;
    return true;
  }

  /**
   * @brief Check if the plugin's subcommand was used.
   *
   * @return true if the subcommand was used.
   */
  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  /**
   * @brief Run the reboot operation.
   *
   * @return true if the operation succeeded.
   */
  PLUGIN_SECTION bool run() override {
    Log::info("Rebooting device!!! (reboot target: {})", rebootTarget);

    if (Helper::Android::reboot(rebootTarget))
      Log::println("Reboot command was sent");
    else
      throw Error("Cannot reboot device!");

    return true;
  }

  /**
   * @brief Get the plugin name.
   *
   * @return std::string The plugin name.
   */
  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  /**
   * @brief Get the plugin version.
   *
   * @return std::string The plugin version.
   */
  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, RebootPlugin)
