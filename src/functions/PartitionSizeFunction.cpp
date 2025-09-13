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

#define SFUN "partitionSizeFunction"
#define FUNCTION_CLASS partitionSizeFunction

namespace PartitionManager {
INIT {
  LOGN(SFUN, INFO)
      << "Initializing variables of partition size getter function."
      << std::endl;
  cmd = _app.add_subcommand("sizeof", "Tell size(s) of input partition list")
            ->footer("Use get-all or getvar-all as partition name for getting "
                     "sizes of all partitions.\nUse get-logicals as partition "
                     "name for getting sizes of logical partitions.\n"
                     "Use get-physical as partition name for getting sizes of "
                     "physical partitions.");
  cmd->add_option("partition(s)", partitions, "Partition name(s).")
      ->required()
      ->delimiter(',');
  cmd->add_flag("--as-byte", asByte,
                "Tell input size of partition list as byte.")
      ->default_val(false);
  cmd->add_flag("--as-kilobyte", asKiloBytes,
                "Tell input size of partition list as kilobyte.")
      ->default_val(false);
  cmd->add_flag("--as-megabyte", asMega,
                "Tell input size of partition list as megabyte.")
      ->default_val(true);
  cmd->add_flag("--as-gigabyte", asGiga,
                "Tell input size of partition list as gigabyte.")
      ->default_val(false);
  cmd->add_flag("--only-size", onlySize,
                "Tell input size of partition list as not printing multiple "
                "and partition name.")
      ->default_val(false);
  return true;
}

RUN {
  sizeCastTypes multiple = {};
  if (asByte) multiple = B;
  if (asKiloBytes) multiple = KB;
  if (asMega) multiple = MB;
  if (asGiga) multiple = GB;

  auto func = [this, &multiple] COMMON_LAMBDA_PARAMS -> bool {
    if (VARS.onLogical && !props.isLogical) {
      if (VARS.forceProcess)
        LOGN(SFUN, WARNING)
            << "Partition " << partition
            << " is exists but not logical. Ignoring (from --force, -f)."
            << std::endl;
      else
        throw Error("Used --logical (-l) flag but is not logical partition: %s",
                    partition.data());
    }

    if (onlySize) println("%d", Helper::convertTo(props.size, multiple));
    else
      println("%s: %d%s", partition.data(),
              Helper::convertTo(props.size, multiple), Helper::multipleToString(multiple).data());

    return true;
  };

  if (partitions.back() == "get-all" || partitions.back() == "getvar-all")
    PART_MAP.doForAllPartitions(func);
  else if (partitions.back() == "get-logicals")
    PART_MAP.doForLogicalPartitions(func);
  else if (partitions.back() == "get-physicals")
    PART_MAP.doForPhysicalPartitions(func);
  else PART_MAP.doForPartitionList(partitions, func);

  return true;
}

IS_USED_COMMON_BODY

NAME { return SFUN; }
} // namespace PartitionManager
