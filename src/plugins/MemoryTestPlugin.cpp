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
#include <cstring>
#include <random>
#include <fcntl.h>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#define PLUGIN "MemoryTestPlugin"
#define PLUGIN_VERSION "1.2"

namespace PartitionManager {

class MemoryTestPlugin final : public BasicPlugin {
  uint64_t bufferSize = MB(4), testFileSize = 0;
  std::filesystem::path testPath;
  bool doNotReadTest = false;

public:
  Helper::CMDLine::Subcommand *cmd = nullptr;
  BasicFlags *flags = nullptr;

  PLUGIN_SECTION MemoryTestPlugin() = default;
  PLUGIN_SECTION ~MemoryTestPlugin() override = default;

  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    cmd = mainApp.addSubcommand("memtest", "Test your write/read speed of device.");
    flags = &mainFlags;
    cmd->addOption("testDirectory", testPath, "Path to test directory")
        ->defaultValue("/data/local/tmp")
        ->check([&](const std::string &val) {
          if (val.find("/sdcard") != std::string::npos || val.find("/storage") != std::string::npos)
            return std::string("Sequential read tests on FUSE-mounted paths do not give correct "
                               "results, so its use is prohibited (by pmt)!");

          if (val != "/data/local/tmp" && !Helper::directoryIsExists(val))
            return std::string("Couldn't find directory: " + val + ", no root? Try executing in ADB shell.");

          return std::string();
        });
    cmd->addOption("-s,--file-size", testFileSize, "File size of test file")
        ->transform(Helper::CMDLine::Transformers::AsSizeValue(false))
        ->defaultValue("1GB");
    cmd->addFlag("--no-read-test", doNotReadTest, "Don't read test data from disk")->defaultValue(false);
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

  PLUGIN_SECTION bool run() override {
    if (testFileSize > GB(2) && !Flags.forceProcess)
      throw Error("File size is more than 2GB! Sizes over 2GB may not give accurate "
                  "results in the write test. Use -f (--force) for skip this error.");

    Log::info("Starting memory test on {}.", testPath.string());
    const std::string test = Helper::pathJoin(testPath, "test.bin");

    Log::info("Generating random data for testing");
    auto buffer = std::make_unique<char[]>(bufferSize);

    for (size_t i = 0; i < bufferSize; i++)
      buffer[i] = static_cast<char>(Helper::Random<1024>::getNumber());

    auto guard = Helper::makeScopeGuard([test] { std::filesystem::remove(test); });

    auto wfd = Helper::UniqueFD(test, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
    if (!wfd) throw Error("Can't open/create test file: {}", strerror(errno));

    Log::info("Sequential write test started!");
    const auto startWrite = std::chrono::high_resolution_clock::now();
    ssize_t bytesWritten = 0;
    while (bytesWritten < testFileSize) {
      const ssize_t ret = wfd.write(buffer.get(), bufferSize);
      if (ret < 0) throw Error("Can't write to test file: {}", strerror(errno));
      bytesWritten += ret;
    }

    const auto endWrite = std::chrono::high_resolution_clock::now();

    const double writeTime = std::chrono::duration<double>(endWrite - startWrite).count();
    Log::println("Sequential write speed: {:3.0f} MB/s", (static_cast<double>(testFileSize) / (1024.0 * 1024.0)) / writeTime);
    Log::info("Sequential write test done!");
    wfd.close();

    if (!doNotReadTest) {
      auto rawBuffer = std::make_unique<char[]>(bufferSize + 4096);
      auto *bufferRead = reinterpret_cast<char *>((reinterpret_cast<uintptr_t>(rawBuffer.get()) + 4096 - 1) & ~(4096 - 1));
      auto rfd = Helper::UniqueFD(test, O_RDONLY | O_DIRECT);
      if (rfd < 0) throw Error("Can't open test file: {}", strerror(errno));

      Log::info("Sequential read test started!");
      const auto startRead = std::chrono::high_resolution_clock::now();
      size_t total = 0;
      ssize_t bytesRead;

      while (true) {
        bytesRead = rfd.read(bufferRead, bufferSize);

        if (bytesRead > 0)
          total += bytesRead;
        else if (bytesRead == 0)
          break;
        else {
          if (errno == EINVAL) {
            int flags_ = rfd.fcntl(F_GETFL);
            if (flags_ != -1) {
              rfd.fcntl(F_SETFL, flags_ & ~O_DIRECT);

              bytesRead = rfd.read(bufferRead, bufferSize);
              if (bytesRead > 0) total += bytesRead;
            }
          } else
            throw Error("Read error during test: {}", strerror(errno));
          break;
        }
      }

      const auto endRead = std::chrono::high_resolution_clock::now();
      const double read_time = std::chrono::duration<double>(endRead - startRead).count();
      Log::println("Sequential read speed: {:3.0f} MB/s", (static_cast<double>(total) / (1024.0 * 1024.0)) / read_time);
      Log::info("Sequential read test done!");
    }

    return true;
  }

  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, MemoryTestPlugin)
