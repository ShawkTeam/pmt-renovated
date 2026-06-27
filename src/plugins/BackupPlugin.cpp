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
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <private/android_filesystem_config.h>

#define PLUGIN "BackupPlugin"
#define PLUGIN_VERSION "1.3"

namespace PartitionManager {

class BackupPlugin final : public BasicPlugin {
  std::vector<std::string> partitions, outputNames;
  std::string rawPartitions, rawOutputNames, outputDirectory;
  uint64_t bufferSize = 0;
  bool noSetPermissions = false, verify = false;

  static constexpr uint64_t MIN_BUFFER_SIZE = 1024;                 ///< 1KB minimum buffer size
  static constexpr uint64_t MAX_BUFFER_SIZE = 128ULL * 1024 * 1024; ///< 128MB maximum buffer size

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  PLUGIN_SECTION BackupPlugin() = default;
  PLUGIN_SECTION ~BackupPlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    flags = &mainFlags;
    cmd = mainApp.addSubcommand("backup", "Backup partition(s) to file(s).");
    cmd->addOption("partition(s)", rawPartitions, "Partition name(s)")->required();
    cmd->addOption("output(s)", rawOutputNames, "File name(s) (or path(s)) to save the partition image(s)");
    cmd->addOption("-O,--output-directory", outputDirectory, "Directory to save the partition image(s)")
        ->check(Helper::CMDLine::Checkers::ExistingDirectory());
    cmd->addOption("-b,--buffer-size", bufferSize, "Buffer size for reading partition(s) and writing to file(s)")
        ->transform(Helper::CMDLine::Transformers::AsSizeValue(false))
        ->defaultValue("1MB")
        ->check(Helper::CMDLine::Checkers::BufferSizeCheck(MIN_BUFFER_SIZE, MAX_BUFFER_SIZE));
    cmd->addFlag("-n,--no-set-perms", noSetPermissions, "Don't change permission and owner after progress")->defaultValue(false);
    cmd->addFlag("-S,--verify", verify, "Verify SHA-256 of the backup image(s)")->defaultValue(false);
    cmd->addFlag("-v,--version", nullptr, "View version of plugin.")
        ->superior()
        ->callback(Helper::CMDLine::Callbacks::ViewPluginVersion(PLUGIN, PLUGIN_VERSION));
    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    Log::info("{}::onUnload() trigger. Bye!", PLUGIN);
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->isUsed(); }

  PLUGIN_SECTION AsyncResult_t runAsync(const std::string &partitionName, const std::string &outputName,
                                        PartitionMap::ProgressRenderer *renderer) const {
    std::optional<PartitionMap::TableType> tType;
    auto *table = getCorrectTableObj(partitionName, Flags.partitionTables.first.get(), Flags.partitionTables.second.get(), tType);
    const PartitionMap::Partition_t *partition = setupPartition(partitionName, table);

    if (!partition) return AsyncResult_t::Error("Couldn't find partition: {}", partitionName);
    if (partition->size() == 0) return AsyncResult_t::Error("Partition {} is empty", partitionName);

    const uint64_t buf = std::clamp<uint64_t>(bufferSize, MIN_BUFFER_SIZE, std::min<uint64_t>(bufferSize, partition->size()));
    Log::info("Backing up {} to {}", partitionName, outputName);

    if (Flags.onLogical && tType != PartitionMap::DYNAMIC) {
      if (Flags.forceProcess)
        Log::warning("Partition {} is exists but not logical. Ignoring (from --force, -f).", partitionName);
      else
        return AsyncResult_t::Error("Used --logical (-l) flag but is not logical partition: {}", partitionName);
    }

    if (Helper::fileIsExists(outputName) && !Flags.forceProcess) {
      return AsyncResult_t::Error("File {} already exists. Remove it, or use --force (-f) flag.", outputName);
    }
    Log::info("Using buffer size (for backing up {}): {}", partitionName, buf);

    std::shared_ptr<PartitionMap::Progress_t> progress;
    if (renderer) progress = renderer->add(partitionName, partition->size());

    std::error_code ec;
    PartitionMap::Partition_t::IOCallback cb = nullptr;
    if (progress) {
      cb = [&progress](uint64_t done, uint64_t) { progress->done.store(done, std::memory_order_relaxed); };
    }

    try {
      partition->dump(outputName, buf, cb);
    } catch (Error &err) {
      if (progress) progress->failed.store(true, std::memory_order_relaxed);
      return AsyncResult_t::Error("Failed to write partition {} to image {}: {}", partitionName, outputName, err.what());
    }

    if (progress) progress->finished.store(true, std::memory_order_relaxed);

    if (verify) {
      if (!Helper::sha256Compare(partition->absolutePath(), outputName)) {
        return AsyncResult_t::Error("Verification failed: {} and {} have different SHA-256 hashes.",
                                    partition->absolutePath().string(), outputName);
      }
      Log::info("SHA-256 verification successful for {}", outputName);
    }

    if (!noSetPermissions) {
      if (!Helper::changeOwner(outputName, AID_EVERYBODY, AID_EVERYBODY))
        Log::info("Failed to change owner of output file: {}. Access problems may occur in non-root users.");
      if (!Helper::changeMode(outputName, DEFAULT_FILE_PERMS))
        Log::info("Failed to change mode of output file to {:o}: {}. Access problems may occur in non-root users.", DEFAULT_FILE_PERMS,
                  outputName);
    }

    return AsyncResult_t::Success("Partition {} successfully backed up to {}", partitionName, outputName);
  }

  PLUGIN_SECTION bool run() override {
    processCommandLine(partitions, outputNames, rawPartitions, rawOutputNames, ',', true);
    if (!outputNames.empty() && partitions.size() != outputNames.size())
      throw Helper::Error("You must provide an output name(s) as long as the partition name(s)").cmdlineError().withCode(EX_USAGE);

    Helper::AsyncManager<AsyncResult_t> manager;
    manager.print = false;
    std::unique_ptr<PartitionMap::ProgressRenderer> renderer;
    if (!Flags.quietProcess) renderer = std::make_unique<PartitionMap::ProgressRenderer>();

    for (size_t i = 0; i < partitions.size(); i++) {
      std::string partitionName = partitions[i];
      std::string outputName = outputNames.empty() ? partitionName + ".img" : outputNames[i];
      if (!outputDirectory.empty()) outputName.insert(0, outputDirectory + '/');

      manager.addProcess(&BackupPlugin::runAsync, this, partitionName, outputName, renderer.get());
      Log::info("Created thread for backing up {}", partitionName);
    }

    PLUGIN_END_WITH_RENDERER(renderer, manager);
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, BackupPlugin)
