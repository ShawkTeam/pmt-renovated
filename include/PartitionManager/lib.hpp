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

#ifndef LIBPMT_LIB_HPP
#define LIBPMT_LIB_HPP

#include <list>
#include <string>
#include <string_view>
#include <functional>
#include <vector>
#include <libhelper/lib.hpp>
#include <libpartition_map/lib.hpp>

#ifdef NEED_BASIC_FUNCTION_CLASSES
#include <memory>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignore "-Wdeprecated-declarations"
#include <CLI/CLI11.hpp>
#pragma GCC diagnostic pop
#endif // #ifdef NEED_BASIC_FUNCTION_CLASSES

namespace PartitionManager {

#ifdef NEED_BASIC_FUNCTION_CLASSES
class basic_function {
/**
 * Example variables for writing your function:
 * private:
 *  CLI::App _cmd* = nullptr;
 */
public:
	virtual bool init(CLI::App& _app) = 0;
	virtual bool run() = 0;
	virtual const char* name() = 0;
	virtual ~basic_function() = default;
};

class basic_function_manager {
private:
	std::vector<std::unique_ptr<basic_function>> _functions;

public:
	void registerFunction(std::unique_ptr<basic_function> _func, CLI::App& _app);

	void startUsedFunctions();
};

using FunctionBase = basic_function;
using FunctionManager = basic_function_manager;

#endif // #ifdef NEED_BASIC_FUNCTION_CLASSES

namespace Variables {

extern PartitionMap::BuildMap PartMap;

} // namespace Variables

int Main(int argc, char** argv);

std::string getLibVersion();
std::string getAppVersion();

} // namespace PartitionManager

#endif // #ifndef LIBPMT_LIB_HPP
