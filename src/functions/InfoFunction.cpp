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
#include <nlohmann/json.hpp>

#define IFUN "infoFunction"

namespace PartitionManager {

INIT(infoFunction) {
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
                "be written separately")
      ->default_val(false);
  cmd->add_option("--json-partition-name", jNamePartition,
                  "Specify partition name element for JSON body")
      ->default_val("name");
  cmd->add_option("--json-size-name", jNameSize,
                  "Specify size element name for JSON body")
      ->default_val("size");
  cmd->add_option("--json-logical-name", jNameLogical,
                  "Specify logical element name for JSON body")
      ->default_val("isLogical");
  cmd->add_option("--json-indent-size", jIndentSize,
                  "Set JSON indent size for printing to screen")
      ->default_val(2);
  return true;
}

RUN(infoFunction) {
  if (partitions.back() == "get-all" || partitions.back() == "getvar-all") {
    if (!Variables->PartMap->copyPartitionsToVector(partitions))
      throw Error("Cannot get list of all partitions! See logs for more "
                  "information (%s)",
                  Helper::LoggingProperties::FILE.data());
  }

  std::vector<PartitionMap::Partition_t> jParts;
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
      jParts.push_back({partition,
                        {Variables->PartMap->sizeOf(partition),
                         Variables->PartMap->isLogical(partition)}});
    else
#ifdef __LP64__
      println("partition=%s size=%lu isLogical=%s",
#else
      println("partition=%s size=%llu isLogical=%s",
#endif
              partition.data(), Variables->PartMap->sizeOf(partition),
              Variables->PartMap->isLogical(partition) ? "true" : "false");
  }

  if (jsonFormat) {
    nlohmann::json j;
    j["partitions"] = nlohmann::json::array();
    for (const auto &[name, props] : jParts) {
      j["partitions"].push_back({{jNamePartition, name},
                                 {jNameSize, props.size},
                                 {jNameLogical, props.isLogical}});
    }

    println("%s", j.dump(jIndentSize).data());
  }

  return true;
}

IS_USED_COMMON_BODY(infoFunction)

NAME(infoFunction) { return IFUN; };
} // namespace PartitionManager
