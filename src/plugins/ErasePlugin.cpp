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
#include <future>
#include <unistd.h>
#include <fcntl.h>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <CLI11.hpp>

#define PLUGIN "ErasePlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class ErasePlugin final : public BasicPlugin {
  std::vector<std::string> partitions;
  uint64_t bufferSize = 0;

public:
  CLI::App *cmd = nullptr;
  BasicFlags *flags;
  const char *logPath = nullptr;

  PLUGIN_SECTION ErasePlugin() DEFAULT_PLUGIN_CONSTRUCTOR;
  PLUGIN_SECTION ~ErasePlugin() override = default;

  PLUGIN_SECTION bool onLoad(CLI::App &mainApp, const std::string &logpath, BasicFlags &mainFlags) override {
    logPath = logpath.c_str();
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.add_subcommand("erase", "Writes zero bytes to partition(s)");
    flags = &mainFlags;
    cmd->add_option("partition(s)", partitions, "Partition name(s)")->required()->delimiter(',');
    cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for writing zero bytes to partition(s)")
        ->transform(CLI::AsSizeValue(false))
        ->default_val("4KB");

    return true;
  }

  PLUGIN_SECTION bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  PLUGIN_SECTION bool used() override { return cmd->parsed(); }

  PLUGIN_SECTION AsyncResult_t runAsync(const std::string &partitionName) const {
    if (!Tables.hasPartition(partitionName)) return AsyncResult_t::Error("Couldn't find partition: {}", partitionName);

    if (Flags.onLogical && !Tables.isLogical(partitionName)) {
      if (Flags.forceProcess)
        LOGNF(PLUGIN, logPath, WARNING) << "Partition " << partitionName << " is exists but not logical. Ignoring (from --force, -f)."
                                        << std::endl;
      else
        return AsyncResult_t::Error("Used --logical (-l) flag but is not logical partition: {}", partitionName);
    }

    uint64_t buf = bufferSize;
    setupBufferSize(buf, partitionName, Tables);

    LOGNF(PLUGIN, logPath, INFO) << "Using buffer size: " << buf;

    // Automatically close file descriptors and delete allocated memories (arrays)
    Helper::garbageCollector collector;
    const auto &partition = Tables.partitionWithDupCheck(partitionName, Flags.noWorkOnUsed);

    auto pfd = Helper::UniqueFD(partition.absolutePath(), O_WRONLY);
    if (!pfd) return AsyncResult_t::Error("Can't open partition: {}: {}", partitionName, strerror(errno));

    if (!Flags.forceProcess) {
      if (!Helper::confirmPropt("Are you sure you want to continue? This could render your device "
                                "unusable! Do not continue if you "
                                "do not know what you are doing!"))
        throw Error("Operation canceled.");
    }

    LOGNF(PLUGIN, logPath, INFO) << "Writing zero bytes to partition: " << partitionName << std::endl;
    auto *buffer = new (std::nothrow) char[buf];
    collector.delAfterProgress(buffer);
    memset(buffer, 0x00, buf);

    ssize_t bytesWritten = 0;
    const uint64_t partitionSize = partition.size();

    while (bytesWritten < partitionSize) {
      size_t toWrite = sizeof(buffer);
      if (partitionSize - bytesWritten < sizeof(buffer)) toWrite = partitionSize - bytesWritten;

      if (const ssize_t result = pfd.write(buffer, toWrite); result == -1)
        return AsyncResult_t::Error("Can't write zero bytes to partition: {}: {}", partitionName, strerror(errno));
      else
        bytesWritten += result;
    }

    return AsyncResult_t::Success("Successfully wrote zero bytes to the %s partition", partitionName);
  }

  PLUGIN_SECTION bool run() override {
    Helper::AsyncManager<AsyncResult_t> manager;
    for (const auto &partitionName : partitions) {
      manager.addProcess(&ErasePlugin::runAsync, this, partitionName);
      LOGNF(PLUGIN, logPath, INFO) << "Created thread for writing zero bytes to " << partitionName << std::endl;
    }

    LOGNF(PLUGIN, logPath, INFO) << "Operation successfully completed." << std::endl;
    return manager();
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, ErasePlugin)
