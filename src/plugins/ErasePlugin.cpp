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
#include <fcntl.h>
#include <future>
#include <unistd.h>

#define PLUGIN "ErasePlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class ErasePlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  uint64_t bufferSize = 0;

public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;

  ~ErasePlugin() override = default;

  bool onLoad(CLI::App &mainApp, FlagsBase &mainFlags) override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.add_subcommand("erase", "Writes zero bytes to partition(s)");
    flags = mainFlags;
    cmd->add_option("partition(s)", partitions, "Partition name(s)")->required()->delimiter(',');
    cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for writing zero bytes to partition(s)")
        ->transform(CLI::AsSizeValue(false))
        ->default_val("4KB");

    return true;
  }

  bool onUnload() override {
    LOGN(PLUGIN, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    return true;
  }

  bool used() override { return cmd->parsed(); }

  resultPair runAsync(const std::string &partitionName) const {
    if (!TABLES.hasPartition(partitionName)) return PairError("Couldn't find partition: %s", partitionName.data());

    if (FLAGS.onLogical && !TABLES.isLogical(partitionName)) {
      if (FLAGS.forceProcess)
        LOGN(PLUGIN, WARNING) << "Partition " << partitionName << " is exists but not logical. Ignoring (from --force, -f)."
                              << std::endl;
      else
        return PairError("Used --logical (-l) flag but is not logical partition: %s", partitionName.data());
    }

    uint64_t buf = bufferSize;
    setupBufferSize(buf, partitionName, *TABLES_REF);

    LOGN(PLUGIN, INFO) << "Using buffer size: " << buf;

    // Automatically close file descriptors and delete allocated memories (arrays)
    Helper::garbageCollector collector;
    auto &partition = TABLES.partitionWithDupCheck(partitionName, FLAGS.noWorkOnUsed);

    const int pfd = Helper::openAndAddToCloseList(partition.getAbsolutePath(), collector, O_WRONLY);
    if (pfd < 0) return PairError("Can't open partition: %s: %s", partitionName.data(), strerror(errno));

    if (!FLAGS.forceProcess) {
      if (!Helper::confirmPropt("Are you sure you want to continue? This could render your device "
                                "unusable! Do not continue if you "
                                "do not know what you are doing!"))
        throw Error("Operation canceled.");
    }

    LOGN(PLUGIN, INFO) << "Writing zero bytes to partition: " << partitionName << std::endl;
    auto *buffer = new (std::nothrow) char[buf];
    collector.delAfterProgress(buffer);
    memset(buffer, 0x00, buf);

    ssize_t bytesWritten = 0;
    const uint64_t partitionSize = partition.getSize();

    while (bytesWritten < partitionSize) {
      size_t toWrite = sizeof(buffer);
      if (partitionSize - bytesWritten < sizeof(buffer)) toWrite = partitionSize - bytesWritten;

      if (const ssize_t result = write(pfd, buffer, toWrite); result == -1)
        return PairError("Can't write zero bytes to partition: %s: %s", partitionName.data(), strerror(errno));
      else
        bytesWritten += result;
    }

    return PairSuccess("Successfully wrote zero bytes to the %s partition", partitionName.data());
  }

  bool run() override {
    std::vector<std::future<resultPair>> futures;
    for (const auto &partitionName : partitions) {
      futures.push_back(std::async(std::launch::async, &ErasePlugin::runAsync, this, partitionName));
      LOGN(PLUGIN, INFO) << "Created thread for writing zero bytes to " << partitionName << std::endl;
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
REGISTER_BUILTIN_PLUGIN(PartitionManager, ErasePlugin);
#else
extern "C" PartitionManager::BasicPlugin *create_plugin() { return new PartitionManager::ErasePlugin(); }
#endif // #ifdef BUILTIN_PLUGINS
