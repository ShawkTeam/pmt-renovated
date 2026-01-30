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

#include <PartitionManager/PartitionManager.hpp>
#include <csignal>
#include <cstdio>
#include <cstdlib>

#include "functions/functions.hpp"
#ifndef ANDROID_BUILD
#include <generated/buildInfo.hpp>
#endif
#include <unistd.h>

#include <string>

namespace PartitionManager {

/**
 * Register functions. Uses ready 'FuncManager' variable.
 *
 * Usage: REGISTER_FUNCTION(FUNCTION_CLASS);
 */
#define REGISTER_FUNCTION(cls) FuncManager.registerFunction(std::make_unique<cls>(), AppMain)

basic_variables::basic_variables()
    : logFile(Helper::LoggingProperties::FILE), onLogical(false), quietProcess(false), verboseMode(false), viewVersion(false),
      forceProcess(false), noWorkOnUsed(false) {
  try {
    partitionTables = std::make_unique<PartitionMap::Builder>();
  } catch (std::exception &) {}
}

static void sigHandler(const int sig) {
  if (sig == SIGINT) OUT.println("\n%sInterrupted.%s", YELLOW, STYLE_RESET);
  if (sig == SIGABRT) OUT.println("\n%sAborted.%s", RED, STYLE_RESET);
  exit(sig);
}

__attribute__((constructor)) void init() {
  Helper::LoggingProperties::setLoggingState<YES>();
  Helper::LoggingProperties::setProgramName(PMTE);
  Helper::LoggingProperties::setLogFile("/sdcard/Documents/last_pmt_logs.log");
}

auto Variables = std::make_unique<VariableTable>();
auto OUT = OutUtil();

int Main(int argc, char **argv) {
  try {
    // try-catch start

    signal(SIGINT, sigHandler);
    signal(SIGABRT, sigHandler);

    if (!isatty(fileno(stdin))) {
      char buf[128];
      while (fgets(buf, sizeof(buf), stdin) != nullptr) {
        buf[strcspn(buf, "\n")] = 0;
        const char *token = strtok(buf, " \t");
        while (token != nullptr) {
          argv[argc] = strdup(token);
          argc++;
          token = strtok(nullptr, " \t");
        }
      }
    }

    if (argc < 2) {
      OUT.println("Usage: %s [OPTIONS] [SUBCOMMAND]\nUse --help for more information.", argv[0]);
      return EXIT_FAILURE;
    }

    CLI::App AppMain{"Partition Manager Tool"};
    FunctionManager FuncManager;

    AppMain.fallthrough(true);
    AppMain.set_help_all_flag("--help-all", "Print full help message and exit");
    AppMain.footer("Partition Manager Tool is written by YZBruh\n"
                   "This project licensed under "
                   "Apache 2.0 license\nReport "
                   "bugs to https://github.com/ShawkTeam/pmt-renovated/issues");
    AppMain.add_option("-t,--table", VARS.extraTablePaths, "Add more partition tables for progress")->delimiter(',');
    AppMain.add_option("-L,--log-file", VARS.logFile, "Set log file");
    AppMain.add_flag("-s,--select-on-duplicate", VARS.noWorkOnUsed,
                     "Select partition for work if has input named duplicate partitions.");
    AppMain.add_flag("-f,--force", VARS.forceProcess, "Force process to be processed");
    AppMain.add_flag("-l,--logical", VARS.onLogical, "Specify that the target partition is logical");
    AppMain.add_flag("-q,--quiet", VARS.quietProcess, "Quiet process");
    AppMain.add_flag("-V,--verbose", VARS.verboseMode,
                     "Detailed information is written on the screen while the "
                     "transaction is "
                     "being carried out");
    AppMain.add_flag("-v,--version", VARS.viewVersion, "Print version and exit");

    REGISTER_FUNCTION(backupFunction);
    REGISTER_FUNCTION(cleanLogFunction);
    REGISTER_FUNCTION(flashFunction);
    REGISTER_FUNCTION(eraseFunction);
    REGISTER_FUNCTION(partitionSizeFunction);
    REGISTER_FUNCTION(infoFunction);
    REGISTER_FUNCTION(realPathFunction);
    REGISTER_FUNCTION(typeFunction);
    REGISTER_FUNCTION(rebootFunction);
    REGISTER_FUNCTION(memoryTestFunction);

    CLI11_PARSE(AppMain, argc, argv);

    Helper::Silencer silencer;
    if (!VARS.quietProcess) silencer.stop();
    if (VARS.verboseMode) Helper::LoggingProperties::setPrinting<YES>();
    if (VARS.viewVersion) {
      OUT.println("%s", getAppVersion().data());
      return EXIT_SUCCESS;
    }

    if (FuncManager.hasFlagOnUsedFunction(NO_MAP_CHECK)) {
      if (!VARS.extraTablePaths.empty())
        WARNING("-t (--tables) flag is ignored. Because, don't needed "
                "partition map by your used function.\n");
      if (VARS.onLogical)
        WARNING("-l (--logical) flag ignored. Because, partition type don't "
                "needed by your used function.\n");
    } else {
      if (!VARS.extraTablePaths.empty())
        std::ranges::for_each(VARS.extraTablePaths, [&](const std::string &name) { PART_TABLES.addTable(name); });
      if (!PART_TABLES && VARS.extraTablePaths.empty())
        throw Error("Can't found any partition table in /dev/block. Specify tables "
                    "-t (--table) argument.");

      if (VARS.onLogical) {
        if (!PART_TABLES.isHasSuperPartition())
          throw Error("This device doesn't contains logical partitions. But you "
                      "used -l (--logical) flag.");
      }
    }

    if (!Helper::hasSuperUser() && !FuncManager.hasFlagOnUsedFunction(NO_SU)) {
      if (!(FuncManager.hasFlagOnUsedFunction(ADB_SUFFICIENT) && Helper::hasAdbPermissions())) {
        throw Error("This function is requires super-user privileges!");
      }
    }

    return FuncManager.handleAll() == true ? EXIT_SUCCESS : EXIT_FAILURE;
  } catch (Helper::Error &error) {
    // catch Helper::Error

    fprintf(OUT.error, "%s%sERROR(S) OCCURRED:%s\n%s\n", RED, BOLD, STYLE_RESET, error.what());
    return EXIT_FAILURE;
  } catch (CLI::Error &error) {
    // catch CLI::Error

    fprintf(stderr, "%s: %s%sFLAG PARSE ERROR:%s %s\n", argv[0], RED, BOLD, STYLE_RESET, error.what());
    return EXIT_FAILURE;
  } // try-catch end
}

std::string getLibVersion() { MKVERSION(PMT); }

std::string getAppVersion() { MKVERSION(PMTE); }
} // namespace PartitionManager
