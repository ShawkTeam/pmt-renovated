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

#include <memory>
#include <PartitionManager/PartitionManager.hpp>
#ifndef ANDROID_BUILD
#include <generated/buildInfo.hpp>
#endif

namespace PartitionManager {

BasicFlags::BasicFlags()
    : logFile(Helper::LoggingProperties::FILE), onLogical(false), quietProcess(false), verboseMode(false), viewVersion(false),
      forceProcess(false), noWorkOnUsed(false) {
  try {
    partitionTables = std::make_unique<PartitionMap::Builder>();
  } catch (...) {
  }
}

__attribute__((constructor)) void init() {
  Helper::LoggingProperties::setLoggingState<YES>();
  Helper::LoggingProperties::setProgramName("pmt");
  Helper::LoggingProperties::setLogFile("/sdcard/Documents/last_pmt_logs.log");
}

std::string getAppVersion() { MKVERSION("pmt"); }

} // namespace PartitionManager
