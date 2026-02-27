/*
 * Copyright (C) 2026 Yağız Zengin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <future>
#include <unistd.h>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <CLI11.hpp>
#include <private/android_filesystem_config.h>

#define PLUGIN "BackupPlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class BackupPlugin final : public BasicPlugin {
  std::vector<std::string> partitions, outputNames;
  std::string rawPartitions, rawOutputNames, outputDirectory;
  uint64_t bufferSize = 0;
  bool noSetPermissions = false;

public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;
  const char *logPath = nullptr;

  PLUGIN_SECTION ~BackupPlugin() override = default;

  PLUGIN_SECTION bool onLoad(CLI::App &mainApp, const std::string &logpath, FlagsBase &mainFlags) override {
    logPath = logpath.c_str();
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    flags = mainFlags;
    cmd = mainApp.add_subcommand("backup", "Backup partition(s) to file(s)");
    cmd->add_option("partition(s)", rawPartitions, "Partition name(s)")->required();
    cmd->add_option("output(s)", rawOutputNames, "File name(s) (or path(s)) to save the partition image(s)");
    cmd->add_option("-O,--output-directory", outputDirectory, "Directory to save the partition image(s)")
        ->check(CLI::ExistingDirectory);
    cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for reading partition(s) and writing to file(s)")
        ->transform(CLI::AsSizeValue(false))
        ->default_val("1MB");
    cmd->add_flag("-n,--no-set-perms", noSetPermissions, "Don't change permission and owner after progress")
        ->default_val(false);
    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->parsed(); }

  PLUGIN_SECTION resultPair runAsync(const std::string &partitionName, const std::string &outputName) const {
    if (!TABLES.hasPartition(partitionName)) return PairError("Couldn't find partition: %s", partitionName.data());
    const auto &partition = TABLES.partitionWithDupCheck(partitionName, FLAGS.noWorkOnUsed);
    const uint64_t buf = std::min<uint64_t>(bufferSize, partition.size());

    LOGNF(PLUGIN, logPath, INFO) << "Back upping " << partitionName << " as " << outputName << std::endl;

    if (FLAGS.onLogical && !TABLES.isLogical(partitionName)) {
      if (FLAGS.forceProcess)
        LOGNF(PLUGIN, logPath, WARNING) << "Partition " << partitionName << " is exists but not logical. Ignoring (from --force, -f)."
                                        << std::endl;
      else
        return PairError("Used --logical (-l) flag but is not logical partition: %s", partitionName.data());
    }

    if (Helper::fileIsExists(outputName) && !FLAGS.forceProcess)
      return PairError("%s is exists. Remove it, or use --force (-f) flag.", outputName.data());

    LOGNF(PLUGIN, logPath, INFO) << "Using buffer size (for back upping " << partitionName << "): " << buf << std::endl;

    try {
      (void)partition.dump(outputName, buf);
    } catch (Helper::Error &error) {
      return PairError("Failed to write %s partition to %s image: %s", partitionName.c_str(), outputName.c_str(), error.what());
    }

    if (!noSetPermissions) {
      if (!Helper::changeOwner(outputName, AID_EVERYBODY, AID_EVERYBODY))
        LOGNF(PLUGIN, logPath, WARNING) << "Failed to change owner of output file: " << outputName
                                        << ". Access problems maybe occur in non-root mode" << std::endl;
      if (!Helper::changeMode(outputName, 0664))
        LOGNF(PLUGIN, logPath, WARNING) << "Failed to change mode of output file as 660: " << outputName
                                        << ". Access problems maybe occur in non-root mode" << std::endl;
    }

    return PairSuccess("%s partition successfully back upped to %s", partitionName.data(), outputName.data());
  }

  PLUGIN_SECTION bool run() override {
    processCommandLine(partitions, outputNames, rawPartitions, rawOutputNames, ',', true);
    if (!outputNames.empty() && partitions.size() != outputNames.size())
      throw CLI::ValidationError("You must provide an output name(s) as long as the partition name(s)");

    Helper::AsyncManager<resultPair> manager;
    for (size_t i = 0; i < partitions.size(); i++) {
      std::string partitionName = partitions[i];
      std::string outputName = outputNames.empty() ? partitionName + ".img" : outputNames[i];
      if (!outputDirectory.empty()) outputName.insert(0, outputDirectory + '/');

      manager.addProcess(&BackupPlugin::runAsync, this, partitionName, outputName);
      LOGNF(PLUGIN, logPath, INFO) << "Created thread backup upping " << partitionName << std::endl;
    }

    return manager();
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, BackupPlugin)
