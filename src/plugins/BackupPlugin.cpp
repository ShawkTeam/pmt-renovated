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

#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <future>
#include <private/android_filesystem_config.h>
#include <unistd.h>

#define PLUGIN "BackupPlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class BackupPlugin final : public BasicPlugin {
  std::vector<std::string> partitions, outputNames;
  std::string rawPartitions, rawOutputNames, outputDirectory;
  uint64_t bufferSize = 0;

public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;

  ~BackupPlugin() override = default;

  bool onLoad(CLI::App &mainApp, FlagsBase &mainFlags) override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    flags = mainFlags;
    cmd = mainApp.add_subcommand("backup", "Backup partition(s) to file(s)");
    cmd->add_option("partition(s)", rawPartitions, "Partition name(s)")->required();
    cmd->add_option("output(s)", rawOutputNames, "File name(s) (or path(s)) to save the partition image(s)");
    cmd->add_option("-O,--output-directory", outputDirectory, "Directory to save the partition image(s)")
        ->check(CLI::ExistingDirectory);
    cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for reading partition(s) and writing to file(s)")
        ->transform(CLI::AsSizeValue(false))
        ->default_val("4KB");
    return true;
  }

  bool onUnload() override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    return true;
  }

  bool used() override { return cmd->parsed(); }

  resultPair runAsync(const std::string &partitionName, const std::string &outputName) const {
    if (!TABLES.hasPartition(partitionName)) return PairError("Couldn't find partition: %s", partitionName.data());
    uint64_t buf = bufferSize;
    setupBufferSize(buf, TABLES.partitionWithDupCheck(partitionName).getAbsolutePath(), *TABLES_REF);

    LOGN(PLUGIN, INFO) << "Back upping " << partitionName << " as " << outputName << std::endl;

    if (FLAGS.onLogical && !TABLES.isLogical(partitionName)) {
      if (FLAGS.forceProcess)
        LOGN(PLUGIN, WARNING) << "Partition " << partitionName << " is exists but not logical. Ignoring (from --force, -f)."
                              << std::endl;
      else
        return PairError("Used --logical (-l) flag but is not logical partition: %s", partitionName.data());
    }

    if (Helper::fileIsExists(outputName) && !FLAGS.forceProcess)
      return PairError("%s is exists. Remove it, or use --force (-f) flag.", outputName.data());

    LOGN(PLUGIN, INFO) << "Using buffer size (for back upping " << partitionName << "): " << buf << std::endl;

    // Automatically close file descriptors and delete allocated memories (arrays)
    Helper::garbageCollector collector;

    const int pfd = Helper::openAndAddToCloseList(TABLES.partitionWithDupCheck(partitionName, FLAGS.noWorkOnUsed).getAbsolutePath(),
                                                  collector, O_RDONLY);
    if (pfd < 0) return PairError("Can't open partition: %s: %s", partitionName.data(), strerror(errno));

    const int ffd = Helper::openAndAddToCloseList(outputName, collector, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (ffd < 0) return PairError("Can't create/open output file %s: %s", outputName.data(), strerror(errno));

    LOGN(PLUGIN, INFO) << "Writing partition " << partitionName << " to file: " << outputName << std::endl;
    auto *buffer = new (std::nothrow) char[buf];
    collector.delAfterProgress(buffer);
    memset(buffer, 0x00, buf);

    ssize_t bytesRead;
    while ((bytesRead = read(pfd, buffer, buf)) > 0) {
      if (const ssize_t bytesWritten = write(ffd, buffer, bytesRead); bytesWritten != bytesRead)
        return PairError("Can't write partition to output file %s: %s", outputName.data(), strerror(errno));
    }

    if (!Helper::changeOwner(outputName, AID_EVERYBODY, AID_EVERYBODY))
      LOGN(PLUGIN, WARNING) << "Failed to change owner of output file: " << outputName
                            << ". Access problems maybe occur in non-root mode" << std::endl;
    if (!Helper::changeMode(outputName, 0664))
      LOGN(PLUGIN, WARNING) << "Failed to change mode of output file as 660: " << outputName
                            << ". Access problems maybe occur in non-root mode" << std::endl;

    return PairSuccess("%s partition successfully back upped to %s", partitionName.data(), outputName.data());
  }

  bool run() override {
    processCommandLine(partitions, outputNames, rawPartitions, rawOutputNames, ',', true);
    if (!outputNames.empty() && partitions.size() != outputNames.size())
      throw CLI::ValidationError("You must provide an output name(s) as long as the partition name(s)");

    std::vector<std::future<resultPair>> futures;
    for (size_t i = 0; i < partitions.size(); i++) {
      std::string partitionName = partitions[i];
      std::string outputName = outputNames.empty() ? partitionName + ".img" : outputNames[i];
      if (!outputDirectory.empty()) outputName.insert(0, outputDirectory + '/');

      futures.push_back(std::async(std::launch::async, &BackupPlugin::runAsync, this, partitionName, outputName));
      LOGN(PLUGIN, INFO) << "Created thread backup upping " << partitionName << std::endl;
    }

    std::string end;
    bool endResult = true;
    for (auto &future : futures) {
      auto [fst, snd] = future.get();
      if (!snd) {
        end += fst + '\n';
        endResult = false;
      } else
        Out::println("%s", fst.c_str());
    }

    if (!endResult) throw Error("%s", end.c_str());

    LOGN(PLUGIN, INFO) << "Operation successfully completed." << std::endl;
    return endResult;
  }

  std::string getName() override { return PLUGIN; }

  std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

#ifdef BUILTIN_PLUGINS
REGISTER_BUILTIN_PLUGIN(PartitionManager, BackupPlugin);
#else
extern "C" PartitionManager::BasicPlugin *create_plugin() { return new PartitionManager::BackupPlugin(); }
#endif // #ifdef BUILTIN_PLUGINS
