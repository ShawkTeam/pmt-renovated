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

#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <PartitionManager/PartitionManager.hpp>
#include <utility>
#include <vector>

namespace PartitionManager {
using pair = std::pair<std::string, bool>;

// Back-up function
class backupFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions, outputNames;
  std::string rawPartitions, rawOutputNames, outputDirectory;
  uint64_t bufferSize = 0;

public:
  CLI::App *cmd = nullptr;

  bool init(CLI::App &_app) override;
  bool run() override;
  static pair runAsync(const std::string &partitionName,
                       const std::string &outputName, uint64_t bufferSize);

  [[nodiscard]] bool isUsed() const override;
  [[nodiscard]] const char *name() const override;
};

// Image flasher function
class flashFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions, imageNames;
  std::string rawPartitions, rawImageNames, imageDirectory;
  uint64_t bufferSize = 0;

public:
  CLI::App *cmd = nullptr;

  bool init(CLI::App &_app) override;
  bool run() override;
  static pair runAsync(const std::string &partitionName,
                       const std::string &imageName, uint64_t bufferSize);

  [[nodiscard]] bool isUsed() const override;
  [[nodiscard]] const char *name() const override;
};

// Eraser function (writes zero bytes to partition)
class eraseFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions;
  uint64_t bufferSize = 0;

public:
  CLI::App *cmd = nullptr;

  bool init(CLI::App &_app) override;
  bool run() override;
  static pair runAsync(const std::string &partitionName, uint64_t bufferSize);

  [[nodiscard]] bool isUsed() const override;
  [[nodiscard]] const char *name() const override;
};

// Partition size getter function
class partitionSizeFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions;
  bool onlySize = false, asByte = false, asKiloBytes = false, asMega = false,
       asGiga = false;

public:
  CLI::App *cmd = nullptr;

  bool init(CLI::App &_app) override;
  bool run() override;

  [[nodiscard]] bool isUsed() const override;
  [[nodiscard]] const char *name() const override;
};

// Partition info getter function
class infoFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions;
  std::string jNamePartition, jNameSize, jNameLogical;
  bool jsonFormat = false;

public:
  CLI::App *cmd = nullptr;

  bool init(CLI::App &_app) override;
  bool run() override;

  [[nodiscard]] bool isUsed() const override;
  [[nodiscard]] const char *name() const override;
};

class realPathFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions;

public:
  CLI::App *cmd = nullptr;

  bool init(CLI::App &_app) override;
  bool run() override;

  [[nodiscard]] bool isUsed() const override;
  [[nodiscard]] const char *name() const override;
};

class realLinkPathFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions;

public:
  CLI::App *cmd = nullptr;

  bool init(CLI::App &_app) override;
  bool run() override;

  [[nodiscard]] bool isUsed() const override;
  [[nodiscard]] const char *name() const override;
};

class typeFunction final : public FunctionBase {
private:
  std::vector<std::string> contents;
  bool onlyCheckAndroidMagics = false, onlyCheckFileSystemMagics = false;
  uint64_t bufferSize = 0;

public:
  CLI::App *cmd = nullptr;

  bool init(CLI::App &_app) override;
  bool run() override;

  [[nodiscard]] bool isUsed() const override;
  [[nodiscard]] const char *name() const override;
};

class rebootFunction final : public FunctionBase {
private:
  std::string rebootTarget;

public:
  CLI::App *cmd = nullptr;

  bool init(CLI::App &_app) override;
  bool run() override;

  [[nodiscard]] bool isUsed() const override;
  [[nodiscard]] const char *name() const override;
};

class memoryTestFunction final : public FunctionBase {
private:
  uint64_t bufferSize = MB(4), /* bufferSizeRandom = KB(4),*/ testFileSize = 0;
  std::string testPath;
  bool doNotWriteTest = false, doNotReadTest = false;

public:
  CLI::App *cmd = nullptr;

  bool init(CLI::App &_app) override;
  bool run() override;

  [[nodiscard]] bool isUsed() const override;
  [[nodiscard]] const char *name() const override;
};

} // namespace PartitionManager

#endif // #ifndef FUNCTIONS_HPP
