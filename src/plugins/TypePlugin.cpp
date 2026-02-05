/*
   Copyright 2025 Yağız Zengin

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

           http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <map>

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

  ~TypePlugin() override = default;

  bool onLoad(CLI::App &mainApp, FlagsBase &mainFlags) override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
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
    LOGN(PLUGIN, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
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
        throw Error("Couldn't find partition or image file: %s", content.data());

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
        throw Error("Couldn't determine type of %s%s", content.data(), content == "userdata" ? " (encrypted file system?)" : "");
    }

    return true;
  }

  std::string getName() override { return PLUGIN; }

  std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

#ifdef BUILTIN_PLUGINS
REGISTER_BUILTIN_PLUGIN(PartitionManager, TypePlugin);
#else
extern "C" PartitionManager::BasicPlugin *create_plugin() { return new PartitionManager::TypePlugin(); }
#endif // #ifdef BUILTIN_PLUGINS
