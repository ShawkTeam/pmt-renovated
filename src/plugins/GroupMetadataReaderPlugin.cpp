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
#include <liblp/metadata_format.h>

#define PLUGIN "GroupMetadataReaderPlugin"
#define PLUGIN_VERSION "1.1"

namespace PartitionManager {

class GroupMetadataReaderPlugin final : public BasicPlugin {
public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  PLUGIN_SECTION GroupMetadataReaderPlugin() = default;
  PLUGIN_SECTION ~GroupMetadataReaderPlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    flags = &mainFlags;
    cmd = mainApp.addSubcommand("read-groups-metadata", "Read logical partition groups metadata.");
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
    auto dTab = GET_DYNAMIC_TABLE_DATA_PTR();
    if (!dTab->isSupported()) throw Error("This device doesn't support dynamic partitions.");

    for (const auto &group : dTab->getGroups()) {
      Log::println("name={} max_size={} flags={}", std::string(group.name), group.maximum_size,
                   (group.flags & LP_GROUP_SLOT_SUFFIXED ? "slot_suffixed" : ""));
    }

    return true;
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
}; // class MetadataReaderPlugin

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, GroupMetadataReaderPlugin)