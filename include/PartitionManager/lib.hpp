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

/**
 * WARNING
 * --------
 * This library (libpmt) isn't exactly suitable for use in different projects.
 * But I'm not saying I've tested it or anything like that.
 */

#ifndef LIBPMT_LIB_HPP
#define LIBPMT_LIB_HPP

#include <list>
#include <string>
#include <string_view>
#include <functional>
#include <vector>
#include <memory>
#include <libhelper/lib.hpp>
#include <libpartition_map/lib.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignore "-Wdeprecated-declarations"
#include <CLI/CLI11.hpp>
#pragma GCC diagnostic pop

#define PMT  "libpmt"
#define PMTE "pmt"
#define PMTF "libpmt-function-manager"

namespace PartitionManager {

/**
 * basic_function
 * --------------
 *   All function classes must inherit from this class.
 */
class basic_function {
public:
	CLI::App* cmd = nullptr;

	virtual bool init(CLI::App& _app) = 0;
	virtual bool run() = 0;
	virtual const char* name() = 0;
	virtual ~basic_function() = default;
};

/**
 * basic_function_manager
 * ----------------------
 *   A class for function management.
 */
class basic_function_manager {
private:
	std::vector<std::unique_ptr<basic_function>> _functions;

public:
	void registerFunction(std::unique_ptr<basic_function> _func, CLI::App& _app);
	const char* whatIsRunnedCommandName();

	bool handleAll();
};

class basic_variables {
public:
	basic_variables();

	PartitionMap::BuildMap PartMap;

	std::string searchPath;
	bool onLogical;
	bool silentProcess;
	bool verboseMode;
	bool viewVersion;
};

using FunctionBase = basic_function;
using FunctionManager = basic_function_manager;
using VariableTable = basic_variables;
using Error = Helper::Error;

VariableTable* Variables;

int Main(int argc, char** argv);

std::string getLibVersion();
std::string getAppVersion(); // Not Android app version (an Android app is planned!), tells pmt version.

} // namespace PartitionManager

#endif // #ifndef LIBPMT_LIB_HPP
