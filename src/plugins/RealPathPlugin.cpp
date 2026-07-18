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
 * @file RealPathPlugin.cpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Implementation of the RealPathPlugin for displaying partition real paths.
 *
 * This file implements the RealPathPlugin class which provides functionality
 * to display the real device paths of partitions.
 */

#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#define PLUGIN "RealPathPlugin"
#define PLUGIN_VERSION "1.2"

namespace PartitionManager {

/**
 * @brief Plugin for displaying partition real paths.
 *
 * This plugin provides functionality to display the real device paths
 * of partitions.
 */
class RealPathPlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  bool byName = false;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  /// @brief Default constructor.
  PLUGIN_SECTION RealPathPlugin() = default;
  /// @brief Default destructor.
  PLUGIN_SECTION ~RealPathPlugin() override = default;

  /**
   * @brief Load the plugin and register its subcommand.
   *
   * @param mainApp The main application instance.
   * @param mainFlags The global flags structure.
   * @return true if the plugin loaded successfully.
   */
  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    cmd = mainApp.addSubcommand("real-path", "Tell real paths of partition(s).");
    flags = &mainFlags;
    cmd->addOption("partition(s)", partitions, "Partition name(s)")->required();
    cmd->addFlag("--by-name", byName, "Print by-name path(s)")->defaultValue(false);
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
   * @brief Run the real path display operation.
   *
   * @return true if the operation succeeded.
   */
  PLUGIN_SECTION bool run() override {
    for (const auto &partition : partitions) {
      std::optional<PartitionMap::TableType> tType;
      auto *table = getCorrectTableObj(partition, Flags.partitionTables.first.get(), Flags.partitionTables.second.get(), tType);
      const PartitionMap::Partition_t *part = setupPartition(partition, table);

      if (!tType && !part) throw Error("Couldn't find partition: {}", partition);
      if (Flags.onLogical && !part->isLogicalPartition()) {
        if (Flags.forceProcess)
          Log::warning("Partition {} is exists but not logical. Ignoring (from --force, -f).", partition);
        else
          throw Error("Used --logical (-l) flag but is not logical partition: {}", partition);
      }

      if (byName)
        Log::println("{}", part->pathByName().string());
      else
        Log::println("{}", part->absolutePath().string());
    }

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

REGISTER_PLUGIN(PartitionManager, RealPathPlugin)
