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

#include <chrono>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "functions.hpp"
#include <PartitionManager/PartitionManager.hpp>

#define MTFUN "memoryTestFunction"

namespace PartitionManager {

bool memoryTestFunction::init(CLI::App &_app) {
  LOGN(MTFUN, INFO) << "Initializing variables of memory test function." << std::endl;
  cmd = _app.add_subcommand("memtest", "Test your write/read speed of device.");
  cmd->add_option("testDirectory", testPath, "Path to test directory")->default_val("/data/local/tmp")->check([&](const std::string &val) {
    if (val != "/data/local/tmp" && !Helper::directoryIsExists(val))
      return std::string("Couldn't find directory: " + val);
    return std::string();
  });
  cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for reading partition(s) and writing to file(s)")->transform(CLI::AsSizeValue(false))->default_val("4KB");
  cmd->add_option("-s,--file-size", testFileSize, "File size of test file")->transform(CLI::AsSizeValue(false))->default_val("1GB");
  cmd->add_flag("--no-write-test", doNotWriteTest, "Don't write test data to disk")->default_val(false);
  cmd->add_flag("--no-read-test", doNotReadTest, "Don't read test data from disk")->default_val(false);

  return true;
}

bool memoryTestFunction::run() {
  if (doNotReadTest && doNotWriteTest)
    throw Error("There must be at least one test transaction, but all of them are blocked");

  LOGN(MTFUN, INFO) << "Starting memory test on " << testPath << std::endl;
  Helper::garbageCollector collector;
  const std::string test = testPath + "/test.bin";

  LOGN(MTFUN, INFO) << "Generating random data for testing" << std::endl;
  auto *buffer = new(std::nothrow) char[bufferSize];
  collector.delAfterProgress(buffer);
  srand(time(nullptr));
  for (size_t i = 0; i < bufferSize; i++) buffer[i] = rand() % 256;

  if (!doNotWriteTest) {
    const int wfd = Helper::openAndAddToCloseList(test, collector, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
    if (wfd < 0) throw Error("Can't open/create test file: %s", strerror(errno));

    LOGN(MTFUN, INFO) << "Write test started!" << std::endl;
    const auto startWrite = std::chrono::high_resolution_clock::now();
    ssize_t bytesWritten = 0;
    while (bytesWritten < testFileSize) {
      const ssize_t ret = write(wfd, buffer, bufferSize);
      if (ret < 0) throw Error("Can't write to test file: %s", strerror(errno));
      bytesWritten += ret;
    }
    const auto endWrite = std::chrono::high_resolution_clock::now();

    const double writeTime = std::chrono::duration<double>(endWrite - startWrite).count();
    println("Write speed: %f MB/s", (testFileSize / (1024.0 * 1024.0)) / writeTime);
    LOGN(MTFUN, INFO) << "Write test done!" << std::endl;
  }

  if (!doNotReadTest) {
    const int rfd = Helper::openAndAddToCloseList(test, collector, O_RDONLY | O_SYNC);
    if (rfd < 0) throw Error("Can't open test file: %s", strerror(errno));

    LOGN(MTFUN, INFO) << "Read test started!" << std::endl;
    const auto startRead = std::chrono::high_resolution_clock::now();
    while (read(rfd, buffer, bufferSize) > 0) {}
    const auto endRead = std::chrono::high_resolution_clock::now();

    const double read_time = std::chrono::duration<double>(endRead - startRead).count();
    println("Read speed: %f MB/s", (testFileSize / (1024.0 * 1024.0)) / read_time);
    LOGN(MTFUN, INFO) << "Read test done!" << std::endl;
  }

  Helper::eraseEntry(test);
  return true;
}

bool memoryTestFunction::isUsed() const { return cmd->parsed(); }

const char *memoryTestFunction::name() const { return MTFUN; }

} // namespace PartitionManager