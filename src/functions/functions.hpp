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

#define NEED_BASIC_FUNCTION_CLASSES
#include <libpmt/lib.hpp>

namespace PartitionManager {

// Back-up function
class backupFunction : public PartitionManager::FunctionBase {
private:
	CLI::App* _cmd = nullptr;

public:
	bool init(CLI::App& _app) override;
	bool run() override;
	const char* name() override;
};

// Image flasher function
class flashFunction : public PartitionManager::FunctionBase {
private:
	CLI::App* _cmd = nullptr;

public:
	bool init(CLI::App& _app) override;
	bool run() override;
	const char* name() override;
};

// Eraser function (only the partition content is cleared)
class eraseFunction : public PartitionManager::FunctionBase {
private:
	CLI::App* _cmd = nullptr;

public:
	bool init(CLI::App& _app) override;
	bool run() override;
	const char* name() override;
};

// Partition size getter function
class partitionSizeFunction : public PartitionManager::FunctionBase {
private:
	CLI::App* _cmd = nullptr;

public:
	bool init(CLI::App& _app) override;
	bool run() override;
	const char* name() override;
};

} // namespace PartitionManager
