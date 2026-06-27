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
#include <libopenpart/openpart.h>

#define PLUGIN "MetadataReaderPlugin"
#define PLUGIN_VERSION "1.1"

namespace PartitionManager {

class MetadataReaderPlugin final : public BasicPlugin {
  std::vector<std::string> partitions;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  PLUGIN_SECTION MetadataReaderPlugin() = default;
  PLUGIN_SECTION ~MetadataReaderPlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    flags = &mainFlags;
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    cmd = mainApp.addSubcommand("read-metadata", "Read logical partition metadata of input partition(s)")
              ->footer("Use get-all or getvar-all as partition name for reading "
                       "all logical partition metadata of all logical partitions.");
    cmd->addOption("partition(s)", partitions, "Partition name(s)")->required();
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

    const auto &tableMetadata = dTab->getMetadata();
    auto reader = [&tableMetadata] FOREACH_LP_METADATA_PARTITION_PARAMETERS_CONST -> bool {
      const auto &group = tableMetadata.groups[metadata.group_index];
      const auto &path = Helper::pathJoin("/dev/block/mapper", metadata.name);
      openpart_t *op = openpart_open(path.c_str(), OP_RDONLY, 0);
      if (!op) {
        Log::error("Failed to open partition: {}", path.string());
        return false;
      }

      uint64_t size = openpart_get_size(op);
      if (size == UINT64_MAX) {
        Log::error("Failed to get size of partition: {}: {}", path.string(), openpart_strerror(op));
        size = 0;
      }

      openpart_close(&op);
      std::vector<std::string> attr_strs;
      attr_strs.reserve(4);
      if (metadata.attributes & LP_PARTITION_ATTR_READONLY) attr_strs.push_back("readonly");
      if (metadata.attributes & LP_PARTITION_ATTR_SLOT_SUFFIXED) attr_strs.push_back("slot_suffixed");
      if (metadata.attributes & LP_PARTITION_ATTR_UPDATED) attr_strs.push_back("updated");
      if (metadata.attributes & LP_PARTITION_ATTR_DISABLED) attr_strs.push_back("disabled");

      Log::print("name={} group={} size={} attributes=", std::string(metadata.name), std::string(group.name), size);
      for (const auto &attr : attr_strs) {
        Log::print("{}", attr);
        if (&attr != &attr_strs.back()) Log::print(",");
      }
      Log::print("\n");

      return true;
    };

    if (partitions.back() == "get-all" || partitions.back() == "getvar-all")
      dTab->forEach(reader);
    else {
      for (const auto &partition : partitions) {
        if (!dTab->hasPartition(partition)) throw Error("Couldn't find logical partition: {}", partition);
      }
      dTab->forEachFor(partitions, reader);
    }

    return true;
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
}; // class MetadataReaderPlugin

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, MetadataReaderPlugin)