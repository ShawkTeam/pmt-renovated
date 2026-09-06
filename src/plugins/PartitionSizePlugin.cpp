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
 * @file PartitionSizePlugin.cpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Implementation of the PartitionSizePlugin for displaying partition sizes.
 *
 * This file implements the PartitionSizePlugin class which provides functionality
 * to display the size of partitions in various units (bytes, KB, MB, GB).
 */

#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#define PLUGIN "PartitionSizePlugin"
#define PLUGIN_VERSION "1.3"

namespace PartitionManager {

/**
 * @brief Plugin for displaying partition sizes.
 *
 * This plugin provides functionality to display the size of partitions
 * in various units (bytes, KB, MB, GB).
 */
class PartitionSizePlugin final : public BasicPlugin {
  enum FormatOptions : uint8_t {
    ONLY_SIZE = 1 << 0,
    AS_BYTE = 1 << 1,
    AS_KILO_BYTE = 1 << 2,
    AS_MEGA_BYTE = 1 << 3,
    AS_GIGA_BYTE = 1 << 4
  };

  std::vector<std::string> partitions;
  Helper::FlagCapsule<FormatOptions> formatOptions;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  /// @brief Default constructor.
  PLUGIN_SECTION PartitionSizePlugin() = default;
  /// @brief Default destructor.
  PLUGIN_SECTION ~PartitionSizePlugin() override = default;

  /**
   * @brief Load the plugin and register its subcommand.
   *
   * @param mainApp The main application instance.
   * @param mainFlags The global flags structure.
   * @return true if the plugin loaded successfully.
   */
  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    using namespace Helper::CMDLine;
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    formatOptions.setFlag(FormatOptions::AS_MEGA_BYTE);
    cmd = mainApp.addSubcommand("sizeof", "Tell size(s) of input partition list.")
              ->footer("Use get-all or getvar-all as partition name for getting "
                       "sizes of all partitions.\nUse get-logicals as partition "
                       "name for getting sizes of logical partitions.\n"
                       "Use get-physical as partition name for getting sizes of "
                       "physical partitions.");
    flags = &mainFlags;
    cmd->addOption("partition(s)", partitions, "Partition name(s).")->required();
    cmd->addFlag("--as-byte", nullptr, "Tell input size of partition list as byte.")
        ->callback(Callbacks::FlagSetter(formatOptions, FormatOptions::AS_BYTE));
    cmd->addFlag("--as-kilobyte", nullptr, "Tell input size of partition list as kilobyte.")
        ->callback(Callbacks::FlagSetter(formatOptions, FormatOptions::AS_KILO_BYTE));
    cmd->addFlag("--as-megabyte", nullptr, "Tell input size of partition list as megabyte.")
        ->callback(Callbacks::FlagSetter(formatOptions, FormatOptions::AS_MEGA_BYTE));
    cmd->addFlag("--as-gigabyte", nullptr, "Tell input size of partition list as gigabyte.")
        ->callback(Callbacks::FlagSetter(formatOptions, FormatOptions::AS_GIGA_BYTE));
    cmd->addFlag("--only-size", nullptr, "Tell input size of partition list as not printing multiple and partition name.")
        ->callback(Callbacks::FlagSetter(formatOptions, FormatOptions::ONLY_SIZE));
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")
        ->strong()
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
   * @brief Run the size display operation.
   *
   * @return true if the operation succeeded.
   */
  PLUGIN_SECTION bool run() override {
    PartitionMap::SizeUnit multiple = {};
    if (formatOptions.hasFlag(FormatOptions::AS_MEGA_BYTE)) multiple = PartitionMap::MiB;
    if (formatOptions.hasFlag(FormatOptions::AS_BYTE)) multiple = PartitionMap::BYTE;
    if (formatOptions.hasFlag(FormatOptions::AS_KILO_BYTE)) multiple = PartitionMap::KiB;
    if (formatOptions.hasFlag(FormatOptions::AS_GIGA_BYTE)) multiple = PartitionMap::GiB;

    auto getter = [this, &multiple] FOREACH_PARTITIONS_LAMBDA_PARAMETERS_CONST -> bool {
      if (formatOptions.hasFlag(FormatOptions::ONLY_SIZE))
        Log::println("{}", partition.formattedSizeString(multiple, true));
      else
        Log::println("{}: {}", partition.name().c_str(), partition.formattedSizeString(multiple));

      return true;
    };

    auto pTab = GET_PARTITION_TABLE_DATA_PTR();
    auto dTab = GET_DYNAMIC_TABLE_DATA_PTR();

    if (partitions.back() == "get-all" || partitions.back() == "getvar-all") {
      pTab->forEach(getter);
      dTab->forEach(getter);
    } else if (partitions.back() == "get-logicals")
      dTab->forEach(getter);
    else if (partitions.back() == "get-physicals")
      pTab->forEach(getter);
    else {
      for (const auto &partition : partitions) {
        if (!pTab->hasPartition(partition) && !dTab->hasPartition(partition))
          throw Error("Couldn't find partition: %s", partition.c_str());
      }
      pTab->forEachFor(partitions, getter);
      dTab->forEachFor(partitions, getter);
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

REGISTER_PLUGIN(PartitionManager, PartitionSizePlugin)
