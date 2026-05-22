/*
 * Copyright (C) 2026 Yağız Zengin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <libhelper/cmdline.hpp>
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#undef Flags

namespace {
constexpr char kInterruptedMessage[] = "\nInterrupted.\n";
constexpr char kAbortedMessage[] = "\nAborted.\n";
} // namespace

static void sigHandler(int sig) {
  if (sig == SIGINT) {
    write(STDERR_FILENO, kInterruptedMessage, sizeof(kInterruptedMessage) - 1);
    exit(128 + SIGINT);
  }
  if (sig == SIGABRT) {
    write(STDERR_FILENO, kAbortedMessage, sizeof(kAbortedMessage) - 1);
    exit(128 + SIGABRT);
  }
}

int main(int argc, char **argv) {
  Helper::CMDLine::App app("Partition Manager Tool");
  std::vector<char *> argvStorage;
  std::vector<std::string> args;
  Helper::Silencer silencer(
      false); // It suppresses stdout and stderr. It redirects them to /dev/null. One of the best ways to run silently.

  try {
    // try-catch start

    signal(SIGINT, sigHandler);  // Trap CTRL+C.
    signal(SIGABRT, sigHandler); // Trap abort signals.

    args.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i)
      args.emplace_back(argv[i]);

    // Catch arguments from stdin.
    if (!isatty(fileno(stdin))) {
      std::string line;
      while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        for (std::string token; iss >> token;)
          args.emplace_back(std::move(token));
      }
    }

    argvStorage.reserve(args.size());
    for (auto &arg : args)
      argvStorage.push_back(arg.data());
    argc = static_cast<int>(argvStorage.size());
    argv = argvStorage.data();

    PartitionManager::BasicFlags Flags;
    std::vector<std::string> plugins;
    std::string pluginPath;

    app.setLicenseString(
        "Copyright (C) 2026 Yağız Zengin\nPartition Manager Tool is written by Yağız Zengin, licensed under GNU GPLv3 license.\nThis "
        "program comes with ABSOLUTELY NO "
        "WARRANTY. Use --license for more information.\nReport "
        "bugs to https://github.com/ShawkTeam/pmt-renovated/issues");

    app.addOption("-L,--log-file", Flags.logFile, "Set log file.")->early();
    app.addOption("-p,--plugins", plugins, "Load input plugin files.")->early();
    app.addOption("-d,--plugin-directory", pluginPath, "Load plugins from the input directory.")
        ->early()
        ->check(Helper::CMDLine::Checkers::ExistingDirectory());

    app.addFlag("-V,--verbose", Flags.verboseMode, "Enable verbose output mode.")->early();
    app.addFlag("-q,--quiet", Flags.quietProcess, "Enable quiet processing.")->early();
    app.addFlag("-s,--select-on-duplicate", Flags.noWorkOnUsed, "Select partition for work if has input named duplicate partitions.");
    app.addFlag("-f,--force", Flags.forceProcess, "Force process to be processed.");
    app.addFlag("-l,--logical", Flags.onLogical, "Specify that the target partition is logical.");
    app.addFlag("-v,--version", Flags.viewVersion, "Print version and exit.");
    app.addFlag("--license", Flags.viewLicense, "Print license and exit.");

    app.parse_earlies(argc, argv);

    Helper::Logger::Properties::setPrinting(Flags.verboseMode);
    Helper::Logger::Properties::setFile(Flags.logFile, true);
    PartitionManager::BasicManager manager(app, Flags.logFile, Flags);

    manager.loadBuiltinPlugins(); // Load built-in plugins if existed.
    if (!plugins.empty()) {
      for (const auto &path : plugins)
        manager.loadPlugin(path); // Load input plugins.
    }
    if (!pluginPath.empty()) {
      for (const auto &entry : std::filesystem::directory_iterator(pluginPath))
        // Try installing all the files with the .so extension in the directory as plugins.
        if (entry.path().extension().string() == ".so") manager.loadPlugin(entry.path().string());
    }

    // AppMain.parse(argc, argv);
    app.parse(argc, argv);

    if (argc < 2 || (argc == 3 && (!plugins.empty() || !pluginPath.empty()))) {
      Out::println("Usage: {} [OPTIONS] [SUBCOMMAND]\nUse --help for more information.", argv[0]);
      return EXIT_FAILURE;
    }

    if (Flags.quietProcess) silencer.silenceAgain();
    if (Flags.viewLicense) {
      Out::println("Copyright (C) 2026 Yağız Zengin\n\nThis program is free software: you can redistribute it and/or modify\nit under "
                   "the terms of the GNU General Public License as published by\nthe Free Software Foundation, either version 3 of "
                   "the License, or\n(at your option) any later version.\n\nThis program is distributed in the hope that it will be "
                   "useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A "
                   "PARTICULAR PURPOSE. See the\nGNU General Public License for more details.\n\nYou should have received a copy of "
                   "the GNU General Public License\nalong with this program.  If not, see <https://www.gnu.org/licenses/>.");
      return EXIT_SUCCESS;
    }
    if (Flags.viewVersion) {
      Out::println("{}", PartitionManager::getAppVersion());
      return EXIT_SUCCESS;
    }

    if (!Helper::Android::isHasRootPrivileges()) // Root access is a fundamental requirement for this program.
      throw PartitionManager::Error("This program requires super-user privileges.");
    if (!Tables) throw PartitionManager::Error("Can't found any partition table in /dev/block.");

    if (Flags.onLogical) {
      if (!Tables.isHasSuperPartition()) // If the device doesn't have a super partition, it means there are no logical partitions.
        throw PartitionManager::Error("This device doesn't contains logical partitions. But you used -l (--logical) flag.");
    }

    if (manager.getUsed().empty()) throw PartitionManager::Error("Unknown main command speficied! Use --help for more information.");

    return !manager.runUsed(); // If the operation is successful, it returns true, which is equal to 1.
  } catch (Helper::Error &error) {
    if (error.isCmdlineError()) {
      fprintf(stderr, "%s: %s\n", argv[0], error.what());
      return error.getErrorCode();
    } else {
      fprintf(stderr, "%s%sFAIL:%s\n%s\n", RED, BOLD, STYLE_RESET, error.what());
      return error.getErrorCode();
    }
  }
}
