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

#define PLUGIN "PartitionSizePlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class PartitionSizePlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  bool onlySize = false, asByte = false, asKiloBytes = false, asMega = false, asGiga = false;

public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;
  const char *logPath = nullptr;

  ~PartitionSizePlugin() override = default;

  PLUGIN_SECTION bool onLoad(CLI::App &mainApp, const std::string &logpath, FlagsBase &mainFlags) override {
    logPath = logpath.c_str();
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.add_subcommand("sizeof", "Tell size(s) of input partition list")
              ->footer("Use get-all or getvar-all as partition name for getting "
                       "sizes of all partitions.\nUse get-logicals as partition "
                       "name for getting sizes of logical partitions.\n"
                       "Use get-physical as partition name for getting sizes of "
                       "physical partitions.");
    flags = mainFlags;
    cmd->add_option("partition(s)", partitions, "Partition name(s).")->required()->delimiter(',');
    cmd->add_flag("--as-byte", asByte, "Tell input size of partition list as byte.")->default_val(false);
    cmd->add_flag("--as-kilobyte", asKiloBytes, "Tell input size of partition list as kilobyte.")->default_val(false);
    cmd->add_flag("--as-megabyte", asMega, "Tell input size of partition list as megabyte.")->default_val(true);
    cmd->add_flag("--as-gigabyte", asGiga, "Tell input size of partition list as gigabyte.")->default_val(false);
    cmd->add_flag("--only-size", onlySize,
                  "Tell input size of partition list as not printing multiple "
                  "and partition name.")
        ->default_val(false);
    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->parsed(); }

  PLUGIN_SECTION bool run() override {
    PartitionMap::SizeUnit multiple = {};
    if (asByte) multiple = PartitionMap::BYTE;
    if (asKiloBytes) multiple = PartitionMap::KiB;
    if (asMega) multiple = PartitionMap::MiB;
    if (asGiga) multiple = PartitionMap::GiB;

    auto getter = [this, &multiple] FOREACH_PARTITIONS_LAMBDA_PARAMETERS_CONST -> bool {
      if (onlySize)
        Out::println("%s", partition.formattedSizeString(multiple, true).c_str());
      else
        Out::println("%s: %s", partition.name().c_str(), partition.formattedSizeString(multiple).c_str());

      return true;
    };

    if (partitions.back() == "get-all" || partitions.back() == "getvar-all")
      TABLES.foreach (getter);
    else if (partitions.back() == "get-logicals")
      TABLES.foreachLogicalPartitions(getter);
    else if (partitions.back() == "get-physicals")
      TABLES.foreachPartitions(getter);
    else {
      for (const auto &partition : partitions) {
        if (!TABLES.hasPartition(partition)) throw Error("Couldn't find partition: %s", partition.c_str());
      }
      TABLES.foreachFor(partitions, getter);
    }

    return true;
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, PartitionSizePlugin)
