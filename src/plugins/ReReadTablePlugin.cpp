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

#define PLUGIN "ReReadTablePlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class ReReadTablePlugin : public BasicPlugin {
  std::vector<std::string> table_names;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  PLUGIN_SECTION ReReadTablePlugin() = default;
  PLUGIN_SECTION ~ReReadTablePlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    flags = &mainFlags;
    cmd = mainApp.addSubcommand("re-read-table", "Re-read input partition table(s).");
    cmd->addOption("table(s)", table_names, "Input partition table name(s).")->required();
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
    auto pTab = GET_PARTITION_TABLE_DATA_PTR();

    for (const auto &name : table_names) {
      if (pTab->hasTable(name)) {
        auto final = std::string("/dev/block/" + name);
        Log::info("Closing opened file descriptors by libpartition_map before reading {}.", final);
        pTab->forEach([&name] FOREACH_PARTITIONS_LAMBDA_PARAMETERS {
          if (partition.tableName() == name) {
            if (!partition.closeFdNow())
              Log::warning("Cannot close {} (fd {}): {}", partition.path().string(), openpart_get_fd(partition.getOpenPart()),
                           strerror(errno));
          }
          return true;
        });

        Log::print("{} is exists ({}), re-reading... ", name, final);

        if (PartitionMap::Extra::reReadTable(final))
          Log::println("Success!");
        else {
          Log::print("Failed!");

          if (Flags.forceProcess)
            Log::println(" Skipping...");
          else {
            Log::print("\n");
            return false;
          }
        }
      } else {
        Log::print("{} table is not exists.", name);

        if (Flags.forceProcess)
          Log::println(" Skipping...");
        else {
          Log::print("\n");
          return false;
        }
      }
    }

    return true;
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, ReReadTablePlugin)
