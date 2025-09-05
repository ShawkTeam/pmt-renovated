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
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <random>
#include <unistd.h>

#define MTFUN "memoryTestFunction"
#define FUNCTION_CLASS memoryTestFunction

namespace PartitionManager {

INIT {
  LOGN(MTFUN, INFO) << "Initializing variables of memory test function."
                    << std::endl;
  cmd = _app.add_subcommand("memtest", "Test your write/read speed of device.");
  cmd->add_option("testDirectory", testPath, "Path to test directory")
      ->default_val("/data/local/tmp")
      ->check([&](const std::string &val) {
        if (val.find("/sdcard") != std::string::npos ||
            val.find("/storage") != std::string::npos)
          return std::string(
              "Sequential read tests on FUSE-mounted paths do not give correct "
              "results, so its use is prohibited (by pmt)!");

        if (val != "/data/local/tmp" && !Helper::directoryIsExists(val))
          return std::string("Couldn't find directory: " + val +
                             ", no root? Try executing in ADB shell.");

        return std::string();
      });
  cmd->add_option("-s,--file-size", testFileSize, "File size of test file")
      ->transform(CLI::AsSizeValue(false))
      ->default_val("1GB");
  cmd->add_flag("--no-read-test", doNotReadTest,
                "Don't read test data from disk")
      ->default_val(false);

  return true;
}

RUN {
  if (testFileSize > GB(2) && !VARS.forceProcess)
    throw Error(
        "File size is more than 2GB! Sizes over 2GB may not give accurate "
        "results in the write test. Use -f (--force) for skip this error.");

  LOGN(MTFUN, INFO) << "Starting memory test on " << testPath << std::endl;
  Helper::garbageCollector collector;
  const std::string test = Helper::pathJoin(testPath, "test.bin");

  LOGN(MTFUN, INFO) << "Generating random data for testing" << std::endl;
  auto *buffer = new (std::nothrow) char[bufferSize];
  collector.delAfterProgress(buffer);
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution dist(0, 255);

  for (size_t i = 0; i < bufferSize; i++)
    buffer[i] = static_cast<char>(dist(rng));

  collector.delFileAfterProgress(test);

  const int wfd = Helper::openAndAddToCloseList(
      test, collector, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
  if (wfd < 0) throw Error("Can't open/create test file: %s", strerror(errno));

  LOGN(MTFUN, INFO) << "Sequential write test started!" << std::endl;
  const auto startWrite = std::chrono::high_resolution_clock::now();
  ssize_t bytesWritten = 0;
  while (bytesWritten < testFileSize) {
    const ssize_t ret = write(wfd, buffer, bufferSize);
    if (ret < 0) throw Error("Can't write to test file: %s", strerror(errno));
    bytesWritten += ret;
  }

  const auto endWrite = std::chrono::high_resolution_clock::now();

  const double writeTime =
      std::chrono::duration<double>(endWrite - startWrite).count();
  println("Sequential write speed: %3.f MB/s",
          (static_cast<double>(testFileSize) / (1024.0 * 1024.0)) / writeTime);
  LOGN(MTFUN, INFO) << "Sequential write test done!" << std::endl;

  if (!doNotReadTest) {
    auto *rawBuffer = new char[bufferSize + 4096];
    collector.delAfterProgress(rawBuffer);
    auto *bufferRead = reinterpret_cast<char *>(
        (reinterpret_cast<uintptr_t>(rawBuffer) + 4096 - 1) & ~(4096 - 1));
    const int rfd =
        Helper::openAndAddToCloseList(test, collector, O_RDONLY | O_DIRECT);
    if (rfd < 0) throw Error("Can't open test file: %s", strerror(errno));

    LOGN(MTFUN, INFO) << "Sequential read test started!" << std::endl;
    const auto startRead = std::chrono::high_resolution_clock::now();
    size_t total = 0;
    ssize_t bytesRead;
    while ((bytesRead = read(rfd, bufferRead, bufferSize)) > 0) {
      total += bytesRead;
    }
    const auto endRead = std::chrono::high_resolution_clock::now();

    const double read_time =
        std::chrono::duration<double>(endRead - startRead).count();
    println("Sequential read speed: %3.f MB/s",
            (static_cast<double>(total) / (1024.0 * 1024.0)) / read_time);
    LOGN(MTFUN, INFO) << "Sequential read test done!" << std::endl;
  }

  return true;
}

IS_USED_COMMON_BODY

NAME { return MTFUN; }
} // namespace PartitionManager
