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

#include "functions/functions.hpp"
#include <PartitionManager/PartitionManager.hpp>
#include <unistd.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <generated/buildInfo.hpp>
#include <string>

namespace PartitionManager {

// Usage: REGISTER_FUNCTION(FUNCTION_CLASS);
#define REGISTER_FUNCTION(cls) \
  FuncManager.registerFunction(std::make_unique<cls>(), AppMain)

basic_variables::basic_variables()
    : logFile(Helper::LoggingProperties::FILE), onLogical(false),
      quietProcess(false), verboseMode(false), viewVersion(false),
      forceProcess(false) {
  try {
    PartMap = std::make_unique<PartitionMap::BuildMap>();
  } catch (std::exception &) {
  }
}

__attribute__((constructor))
void init() {
  Helper::LoggingProperties::setLogFile("/sdcard/Documents/last_pmt_logs.log");
}

static void sigHandler(const int sig) {
  // Even if only SIGINT is to be captured for now, this is still a more appropriate code
  if (sig == SIGINT) println("\n%sInterrupted.%s", YELLOW, STYLE_RESET);
  exit(sig);
}

static int write(void *cookie, const char *buf, const int size) {
  auto *real = static_cast<FILE*>(cookie);
  if (!Variables->quietProcess) return fwrite(buf, 1, static_cast<size_t>(size), real);
  else return size;
}

static FILE* make_fp(FILE* real) {
  return funopen(real, nullptr, write, nullptr, nullptr);
}

auto Variables = std::make_unique<VariableTable>();
FILE* pstdout = make_fp(stdout);
FILE* pstderr = make_fp(stderr);

int Main(int argc, char **argv) {
  try {
    // try-catch start
    Helper::garbageCollector collector;
    collector.closeAfterProgress(pstdout);
    collector.closeAfterProgress(pstderr);

    if (argc < 2) {
      println(
          "Usage: %s [OPTIONS] [SUBCOMMAND]\nUse --help for more information.",
          argv[0]);
      return EXIT_FAILURE;
    }

    signal(SIGINT, sigHandler);

    CLI::App AppMain{"Partition Manager Tool"};
    FunctionManager FuncManager;

    AppMain.fallthrough(true);
    AppMain.set_help_all_flag("--help-all", "Print full help message and exit");
    AppMain.footer("Partition Manager Tool is written by YZBruh\n"
                   "This project licensed under "
                   "Apache 2.0 license\nReport "
                   "bugs to https://github.com/ShawkTeam/pmt-renovated/issues");
    AppMain
        .add_option("-S,--search-path", Variables->searchPath,
                    "Set partition search path")
        ->check([&](const std::string &val) {
          if (val.find("/block") == std::string::npos)
            return std::string(
                "Partition search path is unexpected! Couldn't find "
                "'block' in input path!");
          return std::string();
        });
    AppMain.add_option("-L,--log-file", Variables->logFile, "Set log file");
    AppMain.add_flag("-f,--force", Variables->forceProcess,
                     "Force process to be processed");
    AppMain.add_flag("-l,--logical", Variables->onLogical,
                     "Specify that the target partition is dynamic");
    AppMain.add_flag("-q,--quiet", Variables->quietProcess, "Quiet process");
    AppMain.add_flag("-V,--verbose", Variables->verboseMode,
                     "Detailed information is written on the screen while the "
                     "transaction is "
                     "being carried out");
    AppMain.add_flag("-v,--version", Variables->viewVersion,
                     "Print version and exit");

    REGISTER_FUNCTION(backupFunction);
    REGISTER_FUNCTION(flashFunction);
    REGISTER_FUNCTION(eraseFunction);
    REGISTER_FUNCTION(partitionSizeFunction);
    REGISTER_FUNCTION(infoFunction);
    REGISTER_FUNCTION(realPathFunction);
    REGISTER_FUNCTION(typeFunction);
    REGISTER_FUNCTION(rebootFunction);
    REGISTER_FUNCTION(memoryTestFunction);

    CLI11_PARSE(AppMain, argc, argv);

    if (Variables->verboseMode) Helper::LoggingProperties::setPrinting(YES);
    if (Variables->viewVersion) {
      println("%s", getAppVersion().data());
      return EXIT_SUCCESS;
    }
    if (!Variables->searchPath.empty())
      (*Variables->PartMap)(Variables->searchPath);

    if (!Variables->PartMap && Variables->searchPath.empty())
      throw Error("No default search entries were found. Specify a search "
                  "directory with -S "
                  "(--search-path)");

    if (!Helper::hasSuperUser()) {
      if (!((FuncManager.isUsed("rebootFunction") &&
             Helper::hasAdbPermissions()) ||
            FuncManager.isUsed("memoryTestFunction")))
        throw Error(
            "Partition Manager Tool is requires super-user privileges!\n");
    }

    return FuncManager.handleAll() == true ? EXIT_SUCCESS : EXIT_FAILURE;
  } catch (Helper::Error &error) {
    // catch Helper::Error

    fprintf(pstderr, "%s%sERROR(S) OCCURRED:%s\n%s", RED, BOLD, STYLE_RESET,
              error.what());
    return EXIT_FAILURE;
  } catch (CLI::Error &error) {
    // catch CLI::Error

    fprintf(stderr, "%s: %s%sFLAG PARSE ERROR:%s %s\n", argv[0], RED, BOLD,
            STYLE_RESET, error.what());
    return EXIT_FAILURE;
  } // try-catch block end
}

void print(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(pstdout, format, args);
  va_end(args);
}

void println(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(pstdout, format, args);
  print("\n");
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

std::string getLibVersion() { MKVERSION(PMT); }

std::string getAppVersion() { MKVERSION(PMTE); }
} // namespace PartitionManager
