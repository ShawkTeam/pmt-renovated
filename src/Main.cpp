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
#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#undef FLAGS
#undef TABLES
#define FLAGS (*Flags)
#define TABLES (*FLAGS.partitionTables)

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
  CLI::App AppMain{"Partition Manager Tool"};
  CLI::App bootstrap{"Partition Manager Bootstrap"};
  auto Flags = std::make_shared<PartitionManager::BasicFlags>(); // Generate flag structure.

  try {
    // try-catch start

    signal(SIGINT, sigHandler);  // Trap CTRL+C.
    signal(SIGABRT, sigHandler); // Trap abort signals.

    std::vector<std::string> args;
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

    std::vector<char *> argvStorage;
    argvStorage.reserve(args.size());
    for (auto &arg : args)
      argvStorage.push_back(arg.data());
    argc = static_cast<int>(argvStorage.size());
    argv = argvStorage.data();

    std::vector<std::string> plugins;
    std::string pluginPath;

    // To prevent bootstrap from affecting the main application, disable some of its attributes.
    bootstrap.allow_extras();
    bootstrap.fallthrough(true);
    bootstrap.set_help_flag();
    bootstrap.set_help_all_flag();

    AppMain.allow_extras();
    AppMain.fallthrough(true);
    AppMain.set_help_all_flag("--help-all", "Print full help message and exit");
    AppMain.footer(
        "Copyright (C) 2026 Yağız Zengin\nPartition Manager Tool is written by Yağız Zengin, licensed under GNU GPLv3 license.\nThis "
        "program comes with ABSOLUTELY NO "
        "WARRANTY. Use --license for more information.\nReport "
        "bugs to https://github.com/ShawkTeam/pmt-renovated/issues");
    AppMain.add_option("-t,--table", FLAGS.extraTablePaths, "Add more partition tables for progress.")->delimiter(',');
    AppMain.add_option("-L,--log-file", FLAGS.logFile, "Set log file.");
    AppMain.add_option("-p,--plugins", plugins, "Load input plugin files.")->delimiter(','); // Dummy option for help message.
    AppMain.add_option("-d,--plugin-directory", pluginPath, "Load plugins in input directory.")
        ->check(CLI::ExistingDirectory); // Dummy option for help message.
    AppMain.add_flag("-s,--select-on-duplicate", FLAGS.noWorkOnUsed,
                     "Select partition for work if has input named duplicate partitions.");
    AppMain.add_flag("-f,--force", FLAGS.forceProcess, "Force process to be processed.");
    AppMain.add_flag("-l,--logical", FLAGS.onLogical, "Specify that the target partition is logical.");
    AppMain.add_flag("-q,--quiet", FLAGS.quietProcess, "Quiet process.");
    AppMain.add_flag("-V,--verbose", FLAGS.verboseMode,
                     "Detailed information is written on the screen while the transaction is being carried out.");
    AppMain.add_flag("-v,--version", FLAGS.viewVersion, "Print version and exit.");
    AppMain.add_flag("--license", FLAGS.viewLicense, "Print license and exit.");

    bootstrap.add_option("-p,--plugins", plugins, "Load input plugin files.")->delimiter(',');
    bootstrap.add_option("-d,--plugin-directory", pluginPath, "Load plugins in input directory.")->check(CLI::ExistingDirectory);
    bootstrap.add_option("-L,--log-file", FLAGS.logFile, "Set log file.");

    bootstrap.parse(argc, argv);

    Helper::LoggingProperties::setLogFile(FLAGS.logFile);
    PartitionManager::BasicManager manager(AppMain, FLAGS.logFile, Flags);

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

    AppMain.parse(argc, argv);

    if (argc < 2 || (argc == 3 && (!plugins.empty() || !pluginPath.empty()))) {
      Out::println("Usage: %s [OPTIONS] [SUBCOMMAND]\nUse --help for more information.", argv[0]);
      return EXIT_FAILURE;
    }

    Helper::Silencer
        silencer; // It suppresses stdout and stderr. It redirects them to /dev/null. One of the best ways to run silently.
    if (!FLAGS.quietProcess) silencer.stop();
    if (FLAGS.verboseMode) Helper::LoggingProperties::setPrinting<YES>();
    if (FLAGS.viewLicense) {
      Out::println("Copyright (C) 2026 Yağız Zengin\n\nThis program is free software: you can redistribute it and/or modify\nit under "
                   "the terms of the GNU General Public License as published by\nthe Free Software Foundation, either version 3 of "
                   "the License, or\n(at your option) any later version.\n\nThis program is distributed in the hope that it will be "
                   "useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A "
                   "PARTICULAR PURPOSE. See the\nGNU General Public License for more details.\n\nYou should have received a copy of "
                   "the GNU General Public License\nalong with this program.  If not, see <https://www.gnu.org/licenses/>.");
      return EXIT_SUCCESS;
    }
    if (FLAGS.viewVersion) {
      Out::println("%s", PartitionManager::getAppVersion().data());
      return EXIT_SUCCESS;
    }

    if (!Helper::hasSuperUser()) // Root access is a fundamental requirement for this program.
      throw Helper::Error("This program requires super-user privileges.");
    if (!FLAGS.extraTablePaths.empty()) // If a partition table has been specified for scanning, attempt the load.
      std::ranges::for_each(FLAGS.extraTablePaths, [&](const std::string &name) { TABLES.addTable(name); });
    if (!TABLES && FLAGS.extraTablePaths.empty())
      throw PartitionManager::Error("Can't found any partition table in /dev/block. Specify tables "
                                    "-t (--table) argument.");

    if (FLAGS.onLogical) {
      if (!TABLES.isHasSuperPartition()) // If the device doesn't have a super partition, it means there are no logical partitions.
        throw PartitionManager::Error("This device doesn't contains logical partitions. But you "
                                      "used -l (--logical) flag.");
    }

    return manager.runUsed() == true ? EXIT_SUCCESS : EXIT_FAILURE;
  } catch (CLI::CallForHelp &) {
    // catch CLI::CallForHelp for printing help texts.

    std::cout << AppMain.help() << std::endl;
    return 0;
  } catch (Helper::Error &error) {
    // catch Helper::Error

    fprintf(stderr, "%s%sFAIL:%s\n%s\n", RED, BOLD, STYLE_RESET, error.what());
    return EXIT_FAILURE;
  } catch (CLI::Error &error) {
    // catch CLI::Error

    fprintf(stderr, "%s: %s\n", argv[0], error.what());
    return error.get_exit_code();
  } // try-catch end
}
