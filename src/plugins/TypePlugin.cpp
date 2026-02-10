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

#include <map>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <CLI11.hpp>

#define PLUGIN "TypePlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class TypePlugin final : public BasicPlugin {
  std::vector<std::string> contents;
  bool onlyCheckAndroidMagics = false, onlyCheckFileSystemMagics = false;
  uint64_t bufferSize = 0;

public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;
  const char *logPath = nullptr;

  ~TypePlugin() override = default;

  bool onLoad(CLI::App &mainApp, const std::string &logpath, FlagsBase &mainFlags) override {
    logPath = logpath.c_str();
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.add_subcommand("type", "Get type of the partition(s) or image(s)");
    flags = mainFlags;
    cmd->add_option("content(s)", contents, "Content(s)")->required()->delimiter(',');
    cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for max seek depth")
        ->transform(CLI::AsSizeValue(false))
        ->default_val("4KB");
    cmd->add_flag("--only-check-android-magics", onlyCheckAndroidMagics, "Only check Android magic values.")->default_val(false);
    cmd->add_flag("--only-check-filesystem-magics", onlyCheckFileSystemMagics, "Only check filesystem magic values.")
        ->default_val(false);

    return true;
  }

  bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  bool used() override { return cmd->parsed(); }

  bool run() override {
    std::map<uint64_t, std::string> magics;
    if (onlyCheckAndroidMagics)
      magics.merge(PartitionMap::Extra::AndroidMagics);
    else if (onlyCheckFileSystemMagics)
      magics.merge(PartitionMap::Extra::FileSystemMagics);
    else
      magics.merge(PartitionMap::Extra::Magics);

    for (const auto &content : contents) {
      if (!TABLES.hasPartition(content) && !Helper::fileIsExists(content))
        throw ERR << "Couldn't find partition or image file: " << content;

      bool found = false;
      for (const auto &[magic, name] : magics) {
        if (PartitionMap::Extra::hasMagic(magic, static_cast<ssize_t>(bufferSize),
                                          Helper::fileIsExists(content)
                                              ? content
                                              : TABLES.partitionWithDupCheck(content, FLAGS.noWorkOnUsed).getAbsolutePath().c_str())) {
          Out::println("%s contains %s magic (%s)", content.data(), name.data(), PartitionMap::Extra::formatMagic(magic).data());
          found = true;
          break;
        }
      }

      if (!found)
        throw ERR << "Couldn't determine type of " << content << (content == "userdata" ? " (encrypted filesystem?)" : "");
    }

    return true;
  }

  std::string getName() override { return PLUGIN; }

  std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

#ifdef BUILTIN_PLUGINS
REGISTER_BUILTIN_PLUGIN(PartitionManager, TypePlugin)
#else
REGISTER_DYNAMIC_PLUGIN(PartitionManager::TypePlugin)
#endif // #ifdef BUILTIN_PLUGINS
