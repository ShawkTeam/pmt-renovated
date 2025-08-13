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

std::string convertTo(const uint64_t size, const std::string &multiple) {
  if (multiple == "KB") return std::to_string(TO_KB(size));
  if (multiple == "MB") return std::to_string(TO_MB(size));
  if (multiple == "GB") return std::to_string(TO_GB(size));
  return std::to_string(size);
}

namespace PartitionManager {
bool partitionSizeFunction::init(CLI::App &_app) {
  LOGN(SFUN, INFO)
      << "Initializing variables of partition size getter function."
      << std::endl;
  cmd = _app.add_subcommand("sizeof", "Tell size(s) of input partition list");
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
      ->default_val(false);
  cmd->add_flag("--as-gigabyte", asGiga,
                "Tell input size of partition list as gigabyte.")
      ->default_val(false);
  cmd->add_flag("--only-size", onlySize,
                "Tell input size of partition list as not printing multiple "
                "and partition name.")
      ->default_val(false);
  return true;
}

bool partitionSizeFunction::run() {
  for (const auto &partition : partitions) {
    if (!Variables->PartMap->hasPartition(partition))
      throw Error("Couldn't find partition: %s", partition.data());

    if (Variables->onLogical && !Variables->PartMap->isLogical(partition)) {
      if (Variables->forceProcess)
        LOGN(SFUN, WARNING)
            << "Partition " << partition
            << " is exists but not logical. Ignoring (from --force, -f)."
            << std::endl;
      else
        throw Error("Used --logical (-l) flag but is not logical partition: %s",
                    partition.data());
    }

    std::string multiple = "MB";
    if (asByte) multiple = "B";
    if (asKiloBytes) multiple = "KB";
    if (asMega) multiple = "MB";
    if (asGiga) multiple = "GB";

    if (onlySize)
      println(
          "%s",
          convertTo(Variables->PartMap->sizeOf(partition), multiple).data());
    else
      println("%s: %s%s", partition.data(),
              convertTo(Variables->PartMap->sizeOf(partition), multiple).data(),
              multiple.data());
  }

  return true;
}

bool partitionSizeFunction::isUsed() const { return cmd->parsed(); }

const char *partitionSizeFunction::name() const { return SFUN; }
} // namespace PartitionManager
