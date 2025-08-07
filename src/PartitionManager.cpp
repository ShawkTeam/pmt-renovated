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

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <PartitionManager/PartitionManager.hpp>
#include <generated/buildInfo.hpp>
#include "functions/functions.hpp"

namespace PartitionManager {
	auto Variables = new VariableTable();

	logSetter::logSetter() { Helper::LoggingProperties::setLogFile("/sdcard/Documents/last_pmt_logs.log"); }

	basic_variables::~basic_variables() { delete PartMap; }

	basic_variables::basic_variables() : PartMap(new PartitionMap::BuildMap()),
	                                     logFile("/sdcard/Documents/last_pmt_logs.log"),
	                                     onLogical(false),
	                                     quietProcess(false),
	                                     verboseMode(false),
	                                     viewVersion(false),
	                                     forceProcess(false)
	{}

	int Main(int argc, char **argv) {
		try {
			// try-catch start
			CLI::App AppMain{"Partition Manager Tool"};
			FunctionManager FuncManager;

			AppMain.fallthrough(true);
			AppMain.set_help_all_flag("--help-all", "Print full help message");
			AppMain.footer("Partition Manager Tool is written by YZBruh\nThis project licensed under Apache 2.0 license\nReport bugs to https://github.com/ShawkTeam/pmt-renovated/issues");
			AppMain.add_option("-S,--search-path", Variables->searchPath, "Set partition search path")->check(
				[&](const std::string &val) {
					if (val.find("/block") == std::string::npos) throw CLI::ValidationError(
						"Partition search path is unexpected! Couldn't find 'block' in input path!");
					return std::string();
				});
			AppMain.add_option("-L,--log-file", Variables->logFile, "Set log file");
			AppMain.add_flag("-f,--force", Variables->forceProcess, "Force process to be processed");
			AppMain.add_flag("-l,--logical", Variables->onLogical, "Specify that the target partition is dynamic");
			AppMain.add_flag("-q,--quiet", Variables->quietProcess, "Quiet process");
			AppMain.add_flag("-V,--verbose", Variables->verboseMode,
			                 "Detailed information is written on the screen while the transaction is being carried out");
			AppMain.add_flag("-v,--version", Variables->viewVersion, "Print version and exit");

			if (argc < 2) {
				print("Usage: %s [OPTIONS] [SUBCOMMAND]\nUse --help for more information.\n", argv[0]);
				return EXIT_FAILURE;
			}

			FuncManager.registerFunction(std::make_unique<backupFunction>(), AppMain);
			FuncManager.registerFunction(std::make_unique<flashFunction>(), AppMain);
			FuncManager.registerFunction(std::make_unique<eraseFunction>(), AppMain);
			FuncManager.registerFunction(std::make_unique<partitionSizeFunction>(), AppMain);
			FuncManager.registerFunction(std::make_unique<infoFunction>(), AppMain);
			FuncManager.registerFunction(std::make_unique<realPathFunction>(), AppMain);
			FuncManager.registerFunction(std::make_unique<realLinkPathFunction>(), AppMain);

			CLI11_PARSE(AppMain, argc, argv);

			if (Variables->verboseMode) Helper::LoggingProperties::setPrinting(YES);
			if (Variables->viewVersion) {
				print("%s\n", getAppVersion().data());
				return EXIT_SUCCESS;
			}

			if (!Variables->searchPath.empty()) (*Variables->PartMap)(Variables->searchPath);

			if (!Variables->PartMap && Variables->searchPath.empty())
				throw Error("No default search entries were found. Specify a search directory with -S (--search-path)");

			if (!Helper::hasSuperUser()) throw Error("This program requires super-user privileges!");

			return FuncManager.handleAll() == true ? EXIT_SUCCESS : EXIT_FAILURE;
		} catch (Helper::Error &error) {
			// catch Helper::Error

			if (!Variables->quietProcess) fprintf(stderr, "%s%sERROR(S) OCCURRED%s:\n%s", RED, BOLD, STYLE_RESET,
			                                      error.what());
			delete Variables;
			return EXIT_FAILURE;
		} catch (CLI::Error &error) {
			// catch CLI::Error

			delete Variables;
			fprintf(stderr, "%s: %s%sFLAG PARSE ERROR:%s %s\n", argv[0], RED, BOLD, STYLE_RESET, error.what());
			return EXIT_FAILURE;
		} // try-catch block end
	}

	void print(const char *format, ...) {
		va_list args;
		va_start(args, format);
		if (!Variables->quietProcess) vfprintf(stdout, format, args);
		va_end(args);
	}

	std::string format(const char *format, ...) {
		va_list args;
		va_start(args, format);
		char str[1024];
		vsnprintf(str, sizeof(str), format, args);
		va_end(args);
		return str;
	}

	std::string getLibVersion() {
		MKVERSION(PMT);
	}

	std::string getAppVersion() {
		MKVERSION(PMTE);
	}
} // namespace PartitionManager
