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

#include <fcntl.h>

#include <PartitionManager/PartitionManager.hpp>
#include <cerrno>
#include <cstdlib>
#include <nlohmann/json.hpp>

#include "functions.hpp"

#define IFUN "infoFunction"
#define FUNCTION_CLASS infoFunction

namespace PartitionManager {

INIT {
  LOGN(IFUN, INFO) << "Initializing variables of info printer function." << std::endl;
  cmd = _app.add_subcommand("info", "Tell info(s) of input partition list")
            ->footer("Use get-all or getvar-all as partition name for getting "
                     "info's of all partitions.\nUse get-logicals as partition "
                     "name for getting info's of logical partitions.\n"
                     "Use get-physical as partition name for getting info's of "
                     "physical partitions.");
  cmd->add_option("partition(s)", partitions, "Partition name(s).")->required()->delimiter(',');
  cmd->add_flag("-J,--json", jsonFormat,
                "Print info(s) as JSON body. The body of each partition will "
                "be written separately")
      ->default_val(false);
  cmd->add_flag("--as-byte", asByte, "View sizes as byte.")->default_val(true);
  cmd->add_flag("--as-kilobyte", asKiloBytes, "View sizes as kilobyte.")->default_val(false);
  cmd->add_flag("--as-megabyte", asMega, "View sizes as megabyte.")->default_val(false);
  cmd->add_flag("--as-gigabyte", asGiga, "View sizes as gigabyte.")->default_val(false);
  cmd->add_option("--json-partition-name", jNamePartition, "Specify partition name element for JSON body")->default_val("name");
  cmd->add_option("--json-size-name", jNameSize, "Specify size element name for JSON body")->default_val("size");
  cmd->add_option("--json-logical-name", jNameLogical, "Specify logical element name for JSON body")->default_val("isLogical");
  cmd->add_option("--json-indent-size", jIndentSize, "Set JSON indent size for printing to screen")->default_val(2);
  return true;
}

RUN {
  std::vector<PartitionMap::Partition_t> jParts;
  PartitionMap::SizeUnit multiple;
  if (asByte) multiple = PartitionMap::BYTE;
  if (asKiloBytes) multiple = PartitionMap::KiB;
  if (asMega) multiple = PartitionMap::MiB;
  if (asGiga) multiple = PartitionMap::GiB;

  auto getter = [this, &jParts, &multiple] FOREACH_PARTITIONS_LAMBDA_PARAMETERS -> bool {
    if (jsonFormat)
      jParts.push_back(partition);
    else
      OUT.println("partition=%s size=%s isLogical=%s", partition.getName().c_str(),
                  partition.getFormattedSizeString(multiple, true).c_str(), partition.isLogicalPartition() ? "true" : "false");

    return true;
  };

  if (partitions.back() == "get-all" || partitions.back() == "getvar-all")
    PART_TABLES.foreach (getter);
  else if (partitions.back() == "get-logicals")
    PART_TABLES.foreachLogicalPartitions(getter);
  else if (partitions.back() == "get-physicals")
    PART_TABLES.foreachPartitions(getter);
  else {
    for (const auto& partition : partitions) {
      if (!PART_TABLES.hasPartition(partition)) throw Error("Couldn't find partition: %s", partition.c_str());
    }
    PART_TABLES.foreachFor(partitions, getter);
  }

  if (jsonFormat) {
    nlohmann::json j;
    j["multipleType"] = PartitionMap::Extra::getSizeUnitAsString(multiple);
    j["partitions"] = nlohmann::json::array();
    for (auto &part : jParts)
      j["partitions"].push_back(
          {{jNamePartition, part.getName()}, {jNameSize, part.getSize()}, {jNameLogical, part.isLogicalPartition()}});

    OUT.println("%s", j.dump(jIndentSize).data());
  }

  return true;
}

IS_USED_COMMON_BODY

NAME { return IFUN; }
} // namespace PartitionManager
