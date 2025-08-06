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
#include <vector>

namespace PartitionManager {

// Back-up function
class backupFunction final : public FunctionBase {
private:
	std::vector<std::string> partitions, outputNames;
	std::string rawPartitions, rawOutputNames;
	int bufferSize = 2048;

public:
	CLI::App* cmd = nullptr;

	bool init(CLI::App& _app) override;
	bool run() override;
	[[nodiscard]] bool isUsed() const override;
	[[nodiscard]] const char* name() const override;
};

// Image flasher function
class flashFunction final : public FunctionBase {
private:
	std::vector<std::string> partitions, imageNames;
	std::string rawPartitions, rawImageNames;
	int bufferSize = 2048;

public:
	CLI::App* cmd = nullptr;

	bool init(CLI::App& _app) override;
	bool run() override;
	[[nodiscard]] bool isUsed() const override;
	[[nodiscard]] const char* name() const override;
};

// Eraser function (writes zero bytes to partition)
class eraseFunction final : public FunctionBase {
private:
	std::vector<std::string> partitions;
	int bufferSize = 2048;

public:
	CLI::App* cmd = nullptr;

	bool init(CLI::App& _app) override;
	bool run() override;
	[[nodiscard]] bool isUsed() const override;
	[[nodiscard]] const char* name() const override;
};

// Partition size getter function
class partitionSizeFunction final : public FunctionBase {
private:
	std::vector<std::string> partitions;
	bool onlySize = false, asByte = false, asKiloBytes = false, asMega = true, asGiga = false;

public:
	CLI::App* cmd = nullptr;

	bool init(CLI::App& _app) override;
	bool run() override;
	[[nodiscard]] bool isUsed() const override;
	[[nodiscard]] const char* name() const override;
};

// Partition info getter function
class infoFunction final : public FunctionBase {
private:
	std::vector<std::string> partitions;
	std::string jNamePartition = "name", jNameSize = "size", jNameLogical = "isLogical";
	bool jsonFormat = false;

public:
	CLI::App* cmd = nullptr;

	bool init(CLI::App& _app) override;
	bool run() override;
	[[nodiscard]] bool isUsed() const override;
	[[nodiscard]] const char* name() const override;
};

} // namespace PartitionManager

#endif // #ifndef FUNCTIONS_HPP
