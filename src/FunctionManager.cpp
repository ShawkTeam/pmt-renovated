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

#include <vector>
#include <memory>
#include <string>
#include <PartitionManager/lib.hpp>

namespace PartitionManager {

void basic_function_manager::registerFunction(std::unique_ptr<basic_function> _func, CLI::App& _app)
{
	LOGN(PMTF, INFO) << "registering function: " << _func->name() << std::endl;
	if (!_func->init(_app)) throw Error("Cannot init function: %s\n", _func->name());
	_functions.push_back(std::move(_func));
	LOGN(PMTF, INFO) << _func->name() << " successfully registered." << std::endl;
}

const char* basic_function_manager::whatIsRunnedCommandName()
{
	for (auto &func : _functions) {
		if (func->cmd->parsed()) return func->name();
	}

	return nullptr;
}

bool basic_function_manager::handleAll()
{
	LOGN(PMTF, INFO) << "running catched function commands in command-line." << std::endl;
	for (auto &func : _functions) {
		if (func->cmd->parsed()) {
			LOGN(PMTF, INFO) << "running function: " << func->name() << std::endl;
			return (func->run()) ? true : false;
		}
	}

	return false;
}

} // namespace PartitionManager
