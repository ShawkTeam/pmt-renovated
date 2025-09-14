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
#define FUNCTION_CLASS infoFunction

namespace PartitionManager {

INIT {
  LOGN(IFUN, INFO) << "Initializing variables of info printer function."
                   << std::endl;
  cmd = _app.add_subcommand("info", "Tell info(s) of input partition list")
            ->footer("Use get-all or getvar-all as partition name for getting "
                     "info's of all partitions.\nUse get-logicals as partition "
                     "name for getting info's of logical partitions.\n"
                     "Use get-physical as partition name for getting info's of "
                     "physical partitions.");
  cmd->add_option("partition(s)", partitions, "Partition name(s).")
      ->required()
      ->delimiter(',');
  cmd->add_flag("-J,--json", jsonFormat,
                "Print info(s) as JSON body. The body of each partition will "
                "be written separately")
      ->default_val(false);
  cmd->add_flag("--as-byte", asByte, "View sizes as byte.")->default_val(true);
  cmd->add_flag("--as-kilobyte", asKiloBytes, "View sizes as kilobyte.")
      ->default_val(false);
  cmd->add_flag("--as-megabyte", asMega, "View sizes as megabyte.")
      ->default_val(false);
  cmd->add_flag("--as-gigabyte", asGiga, "View sizes as gigabyte.")
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

RUN {
  std::vector<PartitionMap::Partition_t> jParts;
  sizeCastTypes multiple;
  if (asByte) multiple = B;
  if (asKiloBytes) multiple = KB;
  if (asMega) multiple = MB;
  if (asGiga) multiple = GB;

  auto func = [this, &jParts, &multiple] COMMON_LAMBDA_PARAMS -> bool {
    if (VARS.onLogical && !props.isLogical) {
      if (VARS.forceProcess)
        LOGN(IFUN, WARNING)
            << "Partition " << partition
            << " is exists but not logical. Ignoring (from --force, -f)."
            << std::endl;
      else
        throw Error("Used --logical (-l) flag but is not logical partition: %s",
                    partition.data());
    }

    if (jsonFormat)
      jParts.push_back(
          {partition,
           {static_cast<uint64_t>(Helper::convertTo(props.size, multiple)),
            props.isLogical}});
    else
      println("partition=%s size=%d isLogical=%s", partition.data(),
              Helper::convertTo(props.size, multiple),
              props.isLogical ? "true" : "false");

    return true;
  };

  if (partitions.back() == "get-all" || partitions.back() == "getvar-all")
    PART_MAP.doForAllPartitions(func);
  else if (partitions.back() == "get-logicals")
    PART_MAP.doForLogicalPartitions(func);
  else if (partitions.back() == "get-physicals")
    PART_MAP.doForPhysicalPartitions(func);
  else PART_MAP.doForPartitionList(partitions, func);

  if (jsonFormat) {
    nlohmann::json j;
    j["multipleType"] = Helper::multipleToString(multiple);
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

IS_USED_COMMON_BODY

NAME { return IFUN; };
} // namespace PartitionManager
