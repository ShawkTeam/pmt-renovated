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

#include "functions.hpp"
#include <PartitionManager/PartitionManager.hpp>

#define RFUN "rebootFunction"
#define FUNCTION_CLASS rebootFunction

namespace PartitionManager {
INIT {
  LOGN(RFUN, INFO) << "Initializing variables of reboot function." << std::endl;
  flags = {FunctionFlags::NO_MAP_CHECK, FunctionFlags::ADB_SUFFICIENT};
  cmd = _app.add_subcommand("reboot", "Reboots device");
  cmd->add_option("rebootTarget", rebootTarget,
                  "Reboot target (default: normal)");
  return true;
}

RUN {
  LOGN(RFUN, INFO) << "Rebooting device!!! (custom reboot target: "
                   << (rebootTarget.empty() ? "none" : rebootTarget)
                   << std::endl;

  if (Helper::androidReboot(rebootTarget)) println("Reboot command was sent");
  else throw Error("Cannot reboot device");

  return true;
}

IS_USED_COMMON_BODY

NAME { return RFUN; }
} // namespace PartitionManager
