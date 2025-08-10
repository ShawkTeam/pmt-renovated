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

#define RPFUN "realPathFunction"

namespace PartitionManager {
bool realPathFunction::init(CLI::App &_app) {
  LOGN(RPFUN, INFO) << "Initializing variables of real path function."
                    << std::endl;
  cmd = _app.add_subcommand("real-path", "Tell real paths of partition(s)");
  cmd->add_option("partition(s)", partitions, "Partition name(s)")
      ->required()
      ->delimiter(',');
  return true;
}

bool realPathFunction::run() {
  for (const auto &partition : partitions) {
    if (!Variables->PartMap->hasPartition(partition))
      throw Error("Couldn't find partition: %s", partition.data());

    if (Variables->onLogical && !Variables->PartMap->isLogical(partition)) {
      if (Variables->forceProcess)
        LOGN(RPFUN, WARNING)
            << "Partition " << partition
            << " is exists but not logical. Ignoring (from --force, -f)."
            << std::endl;
      else
        throw Error("Used --logical (-l) flag but is not logical partition: %s",
                    partition.data());
    }

    println("%s", Variables->PartMap->getRealPathOf(partition).data());
  }

  return true;
}

bool realPathFunction::isUsed() const { return cmd->parsed(); }

const char *realPathFunction::name() const { return RPFUN; }
} // namespace PartitionManager
