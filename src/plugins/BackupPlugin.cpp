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

#include <chrono>
#include <fcntl.h>
#include <future>
#include <unistd.h>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <CLI11.hpp>
#include <private/android_filesystem_config.h>

#define PLUGIN "BackupPlugin"
#define PLUGIN_VERSION "1.1"

namespace PartitionManager {

class BackupPlugin final : public BasicPlugin {
  std::vector<std::string> partitions, outputNames;
  std::string rawPartitions, rawOutputNames, outputDirectory;
  uint64_t bufferSize = 0;
  bool noSetPermissions = false, verify = false;

  static constexpr uint64_t MIN_BUFFER_SIZE = 1024;                 ///< 1KB minimum buffer size
  static constexpr uint64_t MAX_BUFFER_SIZE = 128ULL * 1024 * 1024; ///< 128MB maximum buffer size

public:
  CLI::App *cmd = nullptr;
  BasicFlags *flags = nullptr;
  std::string logPath;

  PLUGIN_SECTION BackupPlugin() = default;
  PLUGIN_SECTION ~BackupPlugin() override = default;

  PLUGIN_SECTION bool onLoad(CLI::App &mainApp, const std::string &logpath, BasicFlags &mainFlags) override {
    logPath = logpath;
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    flags = &mainFlags;
    cmd = mainApp.add_subcommand("backup", "Backup partition(s) to file(s)");
    cmd->fallthrough();
    cmd->add_option("partition(s)", rawPartitions, "Partition name(s)")->required();
    cmd->add_option("output(s)", rawOutputNames, "File name(s) (or path(s)) to save the partition image(s)");
    cmd->add_option("-O,--output-directory", outputDirectory, "Directory to save the partition image(s)")
        ->check(CLI::ExistingDirectory);
    cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for reading partition(s) and writing to file(s)")
        ->transform(CLI::AsSizeValue(false))
        ->default_val("1MB")
        ->check([](const std::string &input) -> std::string {
          try {
            uint64_t size = std::stoul(input);
            if (size < MIN_BUFFER_SIZE || size > MAX_BUFFER_SIZE) {
              return "Buffer size must be between 1KB and 128MB";
            }
            return "";
          } catch (...) {
            return "Invalid buffer size format";
          }
        });
    cmd->add_flag("-n,--no-set-perms", noSetPermissions, "Don't change permission and owner after progress")->default_val(false);
    cmd->add_flag("-S,--verify", verify, "Verify SHA-256 of the backup image(s)")->default_val(false);
    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGNF(getName(), logPath, INFO) << getName() << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->parsed(); }

  PLUGIN_SECTION AsyncResult_t runAsync(const std::string &partitionName, const std::string &outputName,
                                        PartitionMap::ProgressRenderer *renderer) const {
    if (!Tables.hasPartition(partitionName)) return AsyncResult_t::Error("Couldn't find partition: {}", partitionName);

    const auto &partition = Tables.partitionWithDupCheck(partitionName)->get();
    if (partition.size() == 0) return AsyncResult_t::Error("Partition {} is empty", partitionName);

    const uint64_t buf = std::clamp<uint64_t>(bufferSize, MIN_BUFFER_SIZE, std::min<uint64_t>(bufferSize, partition.size()));

    LOGNF(PLUGIN, logPath, INFO) << "Backing up " << partitionName << " to " << outputName << std::endl;

    if (Flags.onLogical && !Tables.isLogical(partitionName)) {
      if (Flags.forceProcess)
        LOGNF(PLUGIN, logPath, WARNING) << "Partition " << partitionName << " is exists but not logical. Ignoring (from --force, -f)."
                                        << std::endl;
      else
        return AsyncResult_t::Error("Used --logical (-l) flag but is not logical partition: {}", partitionName);
    }

    if (Helper::fileIsExists(outputName) && !Flags.forceProcess) {
      return AsyncResult_t::Error("File {} already exists. Remove it, or use --force (-f) flag.", outputName);
    }

    LOGNF(PLUGIN, logPath, INFO) << "Using buffer size (for backing up " << partitionName << "): " << buf << std::endl;

    std::shared_ptr<PartitionMap::Progress_t> progress;
    if (renderer) progress = renderer->add(partitionName, partition.size());

    std::error_code ec;
    PartitionMap::Partition_t::IOCallback cb = nullptr;
    if (progress) {
      cb = [&progress](uint64_t done, uint64_t) { progress->done.store(done, std::memory_order_relaxed); };
    }

    if (!partition.dump(ec, outputName, buf, cb)) {
      if (progress) progress->failed.store(true, std::memory_order_relaxed);
      return AsyncResult_t::Error("Failed to write partition {} to image {}: {}", partitionName, outputName, ec.message());
    }
    if (progress) progress->finished.store(true, std::memory_order_relaxed);

    if (verify) {
      if (!Helper::sha256Compare(partition.absolutePath(), outputName)) {
        return AsyncResult_t::Error("Verification failed: {} and {} have different SHA-256 hashes.", partition.absolutePath().string(),
                                    outputName);
      }
      LOGNF(PLUGIN, logPath, INFO) << "SHA-256 verification successful for " << outputName << std::endl;
    }

    if (!noSetPermissions) {
      if (!Helper::changeOwner(outputName, AID_EVERYBODY, AID_EVERYBODY)) {
        LOGNF(PLUGIN, logPath, WARNING) << "Failed to change owner of output file: " << outputName
                                        << ". Access problems may occur in non-root mode" << std::endl;
      }
      if (!Helper::changeMode(outputName, DEFAULT_FILE_PERMS)) {
        LOGNF(PLUGIN, logPath, WARNING) << "Failed to change mode of output file to " << std::oct << DEFAULT_FILE_PERMS << ": "
                                        << outputName << ". Access problems may occur in non-root mode" << std::endl;
      }
    }

    return AsyncResult_t::Success("Partition {} successfully backed up to {}", partitionName, outputName);
  }

  PLUGIN_SECTION bool run() override {
    processCommandLine(partitions, outputNames, rawPartitions, rawOutputNames, ',', true);
    if (!outputNames.empty() && partitions.size() != outputNames.size())
      throw CLI::ValidationError("You must provide an output name(s) as long as the partition name(s)");

    Helper::AsyncManager<AsyncResult_t> manager;
    manager.print = false;
    std::unique_ptr<PartitionMap::ProgressRenderer> renderer;
    if (!Flags.quietProcess) renderer = std::make_unique<PartitionMap::ProgressRenderer>();

    for (size_t i = 0; i < partitions.size(); i++) {
      std::string partitionName = partitions[i];
      std::string outputName = outputNames.empty() ? partitionName + ".img" : outputNames[i];
      if (!outputDirectory.empty()) outputName.insert(0, outputDirectory + '/');

      manager.addProcess(&BackupPlugin::runAsync, this, partitionName, outputName, renderer.get());
      LOGNF(PLUGIN, logPath, INFO) << "Created thread for backing up " << partitionName << std::endl;
    }

    PLUGIN_END_WITH_RENDERER(renderer, manager);
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, BackupPlugin)
