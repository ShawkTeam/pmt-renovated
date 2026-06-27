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

#define PLUGIN "PartitionSizePlugin"
#define PLUGIN_VERSION "1.2"

namespace PartitionManager {

class PartitionSizePlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  bool onlySize = false, asByte = false, asKiloBytes = false, asMega = false, asGiga = false;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  PLUGIN_SECTION PartitionSizePlugin() = default;
  PLUGIN_SECTION ~PartitionSizePlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    cmd = mainApp.addSubcommand("sizeof", "Tell size(s) of input partition list.")
              ->footer("Use get-all or getvar-all as partition name for getting "
                       "sizes of all partitions.\nUse get-logicals as partition "
                       "name for getting sizes of logical partitions.\n"
                       "Use get-physical as partition name for getting sizes of "
                       "physical partitions.");
    flags = &mainFlags;
    cmd->addOption("partition(s)", partitions, "Partition name(s).")->required();
    cmd->addFlag("--as-byte", asByte, "Tell input size of partition list as byte.")->defaultValue(false);
    cmd->addFlag("--as-kilobyte", asKiloBytes, "Tell input size of partition list as kilobyte.")->defaultValue(false);
    cmd->addFlag("--as-megabyte", asMega, "Tell input size of partition list as megabyte.")->defaultValue(true);
    cmd->addFlag("--as-gigabyte", asGiga, "Tell input size of partition list as gigabyte.")->defaultValue(false);
    cmd->addFlag("--only-size", onlySize,
                 "Tell input size of partition list as not printing multiple "
                 "and partition name.")
        ->defaultValue(false);
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")
        ->superior()
        ->callback(Helper::CMDLine::Callbacks::ViewPluginVersion(PLUGIN, PLUGIN_VERSION));
    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    Log::info("{}::onUnload() trigger. Bye!", PLUGIN);
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  PLUGIN_SECTION bool run() override {
    PartitionMap::SizeUnit multiple = {};
    if (asByte) multiple = PartitionMap::BYTE;
    if (asKiloBytes) multiple = PartitionMap::KiB;
    if (asMega) multiple = PartitionMap::MiB;
    if (asGiga) multiple = PartitionMap::GiB;

    auto getter = [this, &multiple] FOREACH_PARTITIONS_LAMBDA_PARAMETERS_CONST -> bool {
      if (onlySize)
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

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, PartitionSizePlugin)
