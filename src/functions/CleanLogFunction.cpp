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

#define CFUN "cleanLogFunction"
#define FUNCTION_CLASS cleanLogFunction

namespace PartitionManager {
INIT {
  LOGN(CFUN, INFO) << "Initializing variables of clean log function." << std::endl;
  flags = {FunctionFlags::NO_MAP_CHECK, FunctionFlags::NO_SU};
  cmd = _app.add_subcommand("clean-logs", "Clean PMT logs.");
  return true;
}

RUN {
  LOGN(CFUN, INFO) << "Removing log file: " << VARS.logFile << std::endl;
  Helper::LoggingProperties::setLoggingState<YES>(); // eraseEntry writes log!
  return Helper::eraseEntry(VARS.logFile);
}

IS_USED_COMMON_BODY

NAME { return CFUN; }

} // namespace PartitionManager
