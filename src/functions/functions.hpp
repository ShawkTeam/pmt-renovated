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

#define INIT(cls) bool cls::init(CLI::App &_app)
#define RUN(cls) bool cls::run()
#define RUN_ASYNC(cls) pair cls::runAsync
#define IS_USED(cls) bool cls::isUsed() const
#define IS_USED_COMMON_BODY(cls)                                               \
  bool cls::isUsed() const { return cmd->parsed(); }
#define NAME(cls) const char *cls::name() const

#define COMMON_LAMBDA_PARAMS                                                   \
  (std::string partition, const PartitionMap::Map_t::BasicInf props)
#define COMMON_FUNCTION_BODY()                                                 \
  CLI::App *cmd = nullptr;                                                     \
  bool init(CLI::App &_app) override;                                          \
  bool run() override;                                                         \
  [[nodiscard]] bool isUsed() const override;                                  \
  [[nodiscard]] const char *name() const override

namespace PartitionManager {
using pair = std::pair<std::string, bool>;

// Back-up function
class backupFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions, outputNames;
  std::string rawPartitions, rawOutputNames, outputDirectory;
  uint64_t bufferSize = 0;

public:
  COMMON_FUNCTION_BODY();
  static pair runAsync(const std::string &partitionName,
                       const std::string &outputName, uint64_t bufferSize);
};

// Image flasher function
class flashFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions, imageNames;
  std::string rawPartitions, rawImageNames, imageDirectory;
  uint64_t bufferSize = 0;

public:
  COMMON_FUNCTION_BODY();
  static pair runAsync(const std::string &partitionName,
                       const std::string &imageName, uint64_t bufferSize);
};

// Eraser function (writes zero bytes to partition)
class eraseFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions;
  uint64_t bufferSize = 0;

public:
  COMMON_FUNCTION_BODY();
  static pair runAsync(const std::string &partitionName, uint64_t bufferSize);
};

// Partition size getter function
class partitionSizeFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions;
  bool onlySize = false, asByte = false, asKiloBytes = false, asMega = false,
       asGiga = false;

public:
  COMMON_FUNCTION_BODY();
};

// Partition info getter function
class infoFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions;
  std::string jNamePartition, jNameSize, jNameLogical;
  int jIndentSize = 2;
  bool jsonFormat = false;

public:
  COMMON_FUNCTION_BODY();
};

class realPathFunction final : public FunctionBase {
private:
  std::vector<std::string> partitions;
  bool realLinkPath = false;

public:
  COMMON_FUNCTION_BODY();
};

class typeFunction final : public FunctionBase {
private:
  std::vector<std::string> contents;
  bool onlyCheckAndroidMagics = false, onlyCheckFileSystemMagics = false;
  uint64_t bufferSize = 0;

public:
  COMMON_FUNCTION_BODY();
};

class rebootFunction final : public FunctionBase {
private:
  std::string rebootTarget;

public:
  COMMON_FUNCTION_BODY();
};

class memoryTestFunction final : public FunctionBase {
private:
  uint64_t bufferSize = MB(4), /* bufferSizeRandom = KB(4),*/ testFileSize = 0;
  std::string testPath;
  bool doNotReadTest = false;

public:
  COMMON_FUNCTION_BODY();
};

} // namespace PartitionManager

#endif // #ifndef FUNCTIONS_HPP
