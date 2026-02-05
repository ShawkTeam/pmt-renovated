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
#include <cstdlib>
#include <cstring>
#include <random>
#include <fcntl.h>
#include <unistd.h>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>
#include <CLI11.hpp>

#define PLUGIN "MemoryTestPlugin"
#define PLUGIN_VERSION "1.0"

namespace PartitionManager {

class MemoryTestPlugin final : public BasicPlugin {
  uint64_t bufferSize = MB(4), testFileSize = 0;
  std::filesystem::path testPath;
  bool doNotReadTest = false;

public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;

  ~MemoryTestPlugin() override = default;

  bool onLoad(CLI::App &mainApp, const std::string& logpath, FlagsBase &mainFlags) override {
    logPath = logpath.c_str();
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onLoad() trigger. Initializing..." << std::endl;
    cmd = mainApp.add_subcommand("memtest", "Test your write/read speed of device.");
    flags = mainFlags;
    cmd->add_option("testDirectory", testPath, "Path to test directory")
        ->default_val("/data/local/tmp")
        ->check([&](const std::string &val) {
          if (val.find("/sdcard") != std::string::npos || val.find("/storage") != std::string::npos)
            return std::string("Sequential read tests on FUSE-mounted paths do not give correct "
                               "results, so its use is prohibited (by pmt)!");

          if (val != "/data/local/tmp" && !Helper::directoryIsExists(val))
            return std::string("Couldn't find directory: " + val + ", no root? Try executing in ADB shell.");

          return std::string();
        });
    cmd->add_option("-s,--file-size", testFileSize, "File size of test file")->transform(CLI::AsSizeValue(false))->default_val("1GB");
    cmd->add_flag("--no-read-test", doNotReadTest, "Don't read test data from disk")->default_val(false);

    return true;
  }

  bool onUnload() override {
    LOGNF(PLUGIN, logPath, INFO) << PLUGIN << "::onUnload() trigger. Bye!" << std::endl;
    cmd = nullptr;
    return true;
  }

  bool used() override { return cmd->parsed(); }

  bool run() override {
    if (testFileSize > GB(2) && !FLAGS.forceProcess)
      throw Error("File size is more than 2GB! Sizes over 2GB may not give accurate "
                  "results in the write test. Use -f (--force) for skip this error.");

    LOGNF(PLUGIN, logPath, INFO) << "Starting memory test on " << testPath << std::endl;
    Helper::garbageCollector collector;
    const std::string test = Helper::pathJoin(testPath, "test.bin");

    LOGNF(PLUGIN, logPath, INFO) << "Generating random data for testing" << std::endl;
    auto *buffer = new (std::nothrow) char[bufferSize];
    collector.delAfterProgress(buffer);

    for (size_t i = 0; i < bufferSize; i++)
      buffer[i] = static_cast<char>(Helper::Random<1024>::getNumber());

    collector.delFileAfterProgress(test);

    const int wfd = Helper::openAndAddToCloseList(test, collector, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
    if (wfd < 0) throw Error("Can't open/create test file: %s", strerror(errno));

    LOGNF(PLUGIN, logPath, INFO) << "Sequential write test started!" << std::endl;
    const auto startWrite = std::chrono::high_resolution_clock::now();
    ssize_t bytesWritten = 0;
    while (bytesWritten < testFileSize) {
      const ssize_t ret = write(wfd, buffer, bufferSize);
      if (ret < 0) throw Error("Can't write to test file: %s", strerror(errno));
      bytesWritten += ret;
    }

    const auto endWrite = std::chrono::high_resolution_clock::now();

    const double writeTime = std::chrono::duration<double>(endWrite - startWrite).count();
    Out::println("Sequential write speed: %3.f MB/s", (static_cast<double>(testFileSize) / (1024.0 * 1024.0)) / writeTime);
    LOGNF(PLUGIN, logPath, INFO) << "Sequential write test done!" << std::endl;

    if (!doNotReadTest) {
      auto *rawBuffer = new char[bufferSize + 4096];
      collector.delAfterProgress(rawBuffer);
      auto *bufferRead = reinterpret_cast<char *>((reinterpret_cast<uintptr_t>(rawBuffer) + 4096 - 1) & ~(4096 - 1));
      const int rfd = Helper::openAndAddToCloseList(test, collector, O_RDONLY | O_DIRECT);
      if (rfd < 0) throw Error("Can't open test file: %s", strerror(errno));

      LOGNF(PLUGIN, logPath, INFO) << "Sequential read test started!" << std::endl;
      const auto startRead = std::chrono::high_resolution_clock::now();
      size_t total = 0;
      ssize_t bytesRead;
      while ((bytesRead = read(rfd, bufferRead, bufferSize)) > 0) {
        total += bytesRead;
      }
      const auto endRead = std::chrono::high_resolution_clock::now();

      const double read_time = std::chrono::duration<double>(endRead - startRead).count();
      Out::println("Sequential read speed: %3.f MB/s", (static_cast<double>(total) / (1024.0 * 1024.0)) / read_time);
      LOGNF(PLUGIN, logPath, INFO) << "Sequential read test done!" << std::endl;
    }

    return true;
  }

  std::string getName() override { return PLUGIN; }

  std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

#ifdef BUILTIN_PLUGINS
REGISTER_BUILTIN_PLUGIN(PartitionManager, MemoryTestPlugin);
#else
extern "C" PartitionManager::BasicPlugin *create_plugin() { return new PartitionManager::MemoryTestPlugin(); }
#endif // #ifdef BUILTIN_PLUGINS
