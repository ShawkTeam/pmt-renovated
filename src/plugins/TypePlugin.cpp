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
 * @file TypePlugin.cpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Implementation of the TypePlugin for detecting partition types.
 *
 * This file implements the TypePlugin class which provides functionality
 * to detect the type of partitions by checking magic numbers for
 * filesystems and Android-specific images.
 */

#include <map>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#define PLUGIN "TypePlugin"
#define PLUGIN_VERSION "1.2"

namespace PartitionManager {

/**
 * @brief Plugin for detecting partition types.
 *
 * This plugin provides functionality to detect the type of partitions by
 * checking magic numbers for filesystems and Android-specific images.
 */
class TypePlugin final : public BasicPlugin {
  std::vector<std::string> contents;
  bool onlyCheckAndroidMagics = false, onlyCheckFileSystemMagics = false;
  uint64_t bufferSize = 0;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  /// @brief Default constructor.
  PLUGIN_SECTION TypePlugin() = default;
  /// @brief Default destructor.
  PLUGIN_SECTION ~TypePlugin() override = default;

  /**
   * @brief Load the plugin and register its subcommand.
   *
   * @param mainApp The main application instance.
   * @param mainFlags The global flags structure.
   * @return true if the plugin loaded successfully.
   */
  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    cmd = mainApp.addSubcommand("type", "Get type of the partition(s) or image(s).");
    flags = &mainFlags;
    cmd->addOption("content(s)", contents, "Content(s)")->required();
    cmd->addOption("-b,--buffer-size", bufferSize, "Buffer size for max seek depth")
        ->transform(Helper::CMDLine::Transformers::AsSizeValue(false))
        ->defaultValue("4KB");
    cmd->addFlag("--only-check-android-magics", onlyCheckAndroidMagics, "Only check Android magic values.")->defaultValue(false);
    cmd->addFlag("--only-check-filesystem-magics", onlyCheckFileSystemMagics, "Only check filesystem magic values.")
        ->defaultValue(false);
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
   * @brief Run the type detection operation.
   *
   * @return true if the operation succeeded.
   */
  PLUGIN_SECTION bool run() override {
    std::map<uint64_t, std::string> magics;
    if (onlyCheckAndroidMagics)
      magics.merge(PartitionMap::Extra::AndroidMagics);
    else if (onlyCheckFileSystemMagics)
      magics.merge(PartitionMap::Extra::FileSystemMagics);
    else
      magics.merge(PartitionMap::Extra::Magics);

    for (const auto &content : contents) {
      std::optional<PartitionMap::TableType> tType;
      auto *table = getCorrectTableObj(content, Flags.partitionTables.first.get(), Flags.partitionTables.second.get(), tType);
      const PartitionMap::Partition_t *partition = setupPartition(content, table);

      if ((!tType && !partition) && !Helper::fileIsExists(content)) throw Error("Couldn't find partition or image file: {}", content);

      bool found = false;
      for (const auto &[magic, name] : magics) {
        if (PartitionMap::Extra::hasMagic(magic, static_cast<ssize_t>(bufferSize),
                                          Helper::fileIsExists(content) ? content : partition->absolutePath().c_str())) {
          Log::println("{} contains {} magic ({})", content, name, PartitionMap::Extra::formatMagic(magic));
          found = true;
          break;
        }
      }

      if (!found) throw Error("Couldn't determine type of {}", content) << (content == "userdata" ? " (encrypted filesystem?)" : "");
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

REGISTER_PLUGIN(PartitionManager, TypePlugin)
