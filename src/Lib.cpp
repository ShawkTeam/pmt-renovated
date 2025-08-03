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
#include <stdio.h>
#include <stdlib.h>
#include <PartitionManager/lib.hpp>
#include <generated/buildInfo.hpp>
#include "functions/functions.hpp"

namespace PartitionManager {

basic_variables::basic_variables()	:	PartMap(),
										searchPath(""),
										onLogical(false),
										silentProcess(false),
										verboseMode(false),
										viewVersion(false)
{}

VariableTable* Variables = new VariableTable();

int Main(int argc, char** argv)
{
try { // try-catch start
	CLI::App AppMain{"Partition Manager Tool"};
	FunctionManager FuncManager;

	AppMain.add_option("-S,--search-path", Variables->searchPath, "Set partition search path");
	AppMain.add_flag("-l,--logical", Variables->onLogical, "Specify that the target partition is dynamic");
	AppMain.add_flag("-s,--silent", Variables->silentProcess, "The process is performed silently without any output.");
	AppMain.add_flag("-V,--verbose", Variables->verboseMode, "Detailed information is written on the screen while the transaction is being carried out");
	AppMain.add_flag("-v,--version", Variables->viewVersion, "Print version and exit");

	FuncManager.registerFunction(std::make_unique<backupFunction>(), AppMain);
	FuncManager.registerFunction(std::make_unique<flashFunction>(), AppMain);
	FuncManager.registerFunction(std::make_unique<eraseFunction>(), AppMain);
	FuncManager.registerFunction(std::make_unique<partitionSizeFunction>(), AppMain);

	CLI11_PARSE(AppMain, argc, argv);

	const char* used = FuncManager.whatIsRunnedCommandName();
	if (used != nullptr)
		LOGN(PMTE, INFO) << "used command: " << used << std::endl;

	if (!Variables->searchPath.empty())
		Variables->PartMap(Variables->searchPath);

	if (!Variables->PartMap) {
		if (Variables->searchPath.empty())
			throw Error("No default search entries were found. Specify a search directory with -S (--search-path)");
	}

	FuncManager.handleAll();

} catch (Helper::Error &error) { // catch Helper::Error

	if (!Variables->silentProcess) fprintf(stderr, "%s: %s.\n", argv[0], error.what());
	delete Variables;
	return -1;

} catch (CLI::ParseError &error) { // catch CLI::ParseError

	fprintf(stderr, "%s: FLAG PARSE ERROR: %s\n", argv[0], error.what());
	return -1;

} // try-catch block end
}

std::string getLibVersion()
{
	char vinfo[512];
	sprintf(vinfo, MKVERSION("libpmt"));
	return std::string(vinfo);
}

std::string getAppVersion()
{
	char vinfo[512];
	sprintf(vinfo, MKVERSION("pmt"));
	return std::string(vinfo);
}

} // namespace PartitionManager
