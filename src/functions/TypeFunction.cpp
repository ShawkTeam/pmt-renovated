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
#include <map>

#include "functions.hpp"

#define TFUN "typeFunction"
#define FUNCTION_CLASS typeFunction

namespace PartitionManager {
INIT {
  LOGN(TFUN, INFO) << "Initializing variables of type function." << std::endl;
  cmd = _app.add_subcommand("type", "Get type of the partition(s) or image(s)");
  cmd->add_option("content(s)", contents, "Content(s)")->required()->delimiter(',');
  cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for max seek depth")
      ->transform(CLI::AsSizeValue(false))
      ->default_val("4KB");
  cmd->add_flag("--only-check-android-magics", onlyCheckAndroidMagics,
                "Only check Android magic values.")
      ->default_val(false);
  cmd->add_flag("--only-check-filesystem-magics", onlyCheckFileSystemMagics,
                "Only check filesystem magic values.")
      ->default_val(false);
  return true;
}

RUN {
  std::map<uint64_t, std::string> magics;
  if (onlyCheckAndroidMagics)
    magics.merge(PartitionMap::Extras::AndroidMagicMap);
  else if (onlyCheckFileSystemMagics)
    magics.merge(PartitionMap::Extras::FileSystemMagicMap);
  else
    magics.merge(PartitionMap::Extras::MagicMap);

  for (const auto &content : contents) {
    if (!PARTS.hasPartition(content) && !Helper::fileIsExists(content))
      throw Error("Couldn't find partition or image file: %s", content.data());

    bool found = false;
    for (const auto &[magic, name] : magics) {
      if (PartitionMap::Extras::hasMagic(
              magic, static_cast<ssize_t>(bufferSize),
              Helper::fileIsExists(content) ? content : PARTS.getRealPathOf(content))) {
        OUT.println("%s contains %s magic (%s)", content.data(), name.data(),
                    PartitionMap::Extras::formatMagic(magic).data());
        found = true;
        break;
      }
    }

    if (!found)
      throw Error("Couldn't determine type of %s%s", content.data(),
                  content == "userdata" ? " (encrypted file system?)" : "");
  }

  return true;
}

IS_USED_COMMON_BODY

NAME { return TFUN; }

} // namespace PartitionManager
