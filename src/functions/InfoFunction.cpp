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
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

#define IFUN "infoFunction"

namespace PartitionManager {
bool infoFunction::init(CLI::App &_app) {
  LOGN(IFUN, INFO) << "Initializing variables of info printer function."
                   << std::endl;
  cmd = _app.add_subcommand("info", "Tell info(s) of input partition list")
            ->footer("Use get-all or getvar-all as partition name for getting "
                     "info's of all "
                     "partitions.");
  cmd->add_option("partition(s)", partitions, "Partition name(s).")
      ->required()
      ->delimiter(',');
  cmd->add_flag("-J,--json", jsonFormat,
                "Print info(s) as JSON body. The body of each partition will "
                "be written separately");
  cmd->add_option("--json-partition-name", jNamePartition,
                  "Speficy partition name element for JSON body");
  cmd->add_option("--json-size-name", jNameSize,
                  "Speficy size element name for JSON body");
  cmd->add_option("--json-logical-name", jNameLogical,
                  "Speficy logical element name for JSON body");
  return true;
}

bool infoFunction::run() {
  if (partitions.back() == "get-all" || partitions.back() == "getvar-all") {
    partitions.clear();
    const auto parts = Variables->PartMap->getPartitionList();
    if (!parts)
      throw Error("Cannot get list of all partitions! See logs for more "
                  "information (%s)",
                  Helper::LoggingProperties::FILE.data());

    for (const auto &name : *parts)
      partitions.push_back(name);
  }

  for (const auto &partition : partitions) {
    if (!Variables->PartMap->hasPartition(partition))
      throw Error("Couldn't find partition: %s", partition.data());

    if (Variables->onLogical && !Variables->PartMap->isLogical(partition)) {
      if (Variables->forceProcess)
        LOGN(IFUN, WARNING)
            << "Partition " << partition
            << " is exists but not logical. Ignoring (from --force, -f)."
            << std::endl;
      else
        throw Error("Used --logical (-l) flag but is not logical partition: %s",
                    partition.data());
    }

    if (jsonFormat)
#ifdef __LP64__
      println("{\"%s\": \"%s\", \"%s\": %lu, \"%s\": %s}",
#else
      println("{\"%s\": \"%s\", \"%s\": %llu, \"%s\": %s}",
#endif
              jNamePartition.data(), partition.data(), jNameSize.data(),
              Variables->PartMap->sizeOf(partition), jNameLogical.data(),
              Variables->PartMap->isLogical(partition) ? "true" : "false");
    else
#ifdef __LP64__
      println("partition=%s size=%lu isLogical=%s",
#else
      println("partition=%s size=%llu isLogical=%s",
#endif
              partition.data(), Variables->PartMap->sizeOf(partition),
              Variables->PartMap->isLogical(partition) ? "true" : "false");
  }

  return true;
}

bool infoFunction::isUsed() const { return cmd->parsed(); }

const char *infoFunction::name() const { return IFUN; };
} // namespace PartitionManager
