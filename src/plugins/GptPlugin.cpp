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

/**
 * @file GptPlugin.cpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Implementation of the GptPlugin for GPT partition table operations.
 *
 * This file implements the GptPlugin class which provides functionality
 * to display GPT partition table information and perform GPT-related operations.
 */

#include <PartitionManager/PartitionManager.hpp>
#include <PartitionManager/Plugin.hpp>

#define PLUGIN "GptPlugin"
#define PLUGIN_VERSION "1.0"

#define SPAWN_ERROR()                                                                                                                 \
  do {                                                                                                                                \
    if (Flags.forceProcess) {                                                                                                         \
      Log::println(" Skipping...");                                                                                                   \
      return true;                                                                                                                    \
    } else {                                                                                                                          \
      Log::print("\n");                                                                                                               \
      return false;                                                                                                                   \
    }                                                                                                                                 \
  } while (0)

#define READ_ALL_HANDLE()                                                                                                             \
  do {                                                                                                                                \
    if (table_names[0] == "read-all") {                                                                                               \
      table_names.assign(pTab->tableNames().begin(), pTab->tableNames().end());                                                       \
    }                                                                                                                                 \
  } while (0)

namespace PartitionManager {

/**
 * @brief Plugin for GPT partition table operations.
 *
 * This plugin provides functionality to display GPT partition table information
 * and perform GPT-related operations such as displaying table details and partition lists.
 */
class GptPlugin final : public BasicPlugin {
public:
  Helper::CMDLine::Subcommand *mainCmd = nullptr;
  Helper::CMDLine::Subcommand *subCmdFirst = nullptr, *subCmdSecond = nullptr, *subCmdThird = nullptr;
  BasicFlags *flags = nullptr;

private:
  std::vector<std::string> table_names, file_list;
  Helper::Silencer silencer = Helper::Silencer(false);
  std::string directory;
  bool use_gpt_backups = false;
  PartitionMap::PartitionTableData *pTab = nullptr;

  /**
   * @brief Re-read the specified partition tables.
   *
   * @return true if successful.
   */
  bool reReadTables() {
    READ_ALL_HANDLE();
    for (const auto &name : table_names) {
      if (pTab->hasTable(name)) {
        auto final = std::string("/dev/block/" + name);
        Log::info("Closing opened file descriptors by libpartition_map before reading {}.", final);
        pTab->forEach([&name] FOREACH_PARTITIONS_LAMBDA_PARAMETERS {
          if (partition.tableName() == name) {
            if (!partition.closeFdNow())
              Log::warning("Cannot close {} (fd {}): {}", partition.path().string(), openpart_get_fd(partition.getOpenPart()),
                           strerror(errno));
          }
          return true;
        });

        Log::print("{} is exists ({}), re-reading... ", name, final);

        if (PartitionMap::Extra::reReadTable(final))
          Log::println("Success!");
        else {
          Log::print("Failed!");
          SPAWN_ERROR();
        }
      }
    }

    return true;
  }

  /**
   * @brief Backup GPT tables to files.
   *
   * @return true if successful.
   */
  bool backupGptTable() {
    if (!file_list.empty() && table_names.size() != file_list.size())
      throw Helper::Error("You must provide an output name(s) as long as the table name(s)").cmdlineError().withCode(EX_USAGE);

    READ_ALL_HANDLE();
    for (size_t i = 0; i < table_names.size(); i++) {
      const auto &n = table_names[i];
      if (!pTab->hasTable(n)) continue;
      std::string output = file_list.empty() ? n + ".gpt" : file_list[i];
      if (!directory.empty()) output.insert(0, directory + '/');

      if (Helper::fileIsExists(output) && !Flags.forceProcess)
        throw Helper::Error("Output file {} already exists!", output).cmdlineError().withCode(EX_DATAERR);

      auto table = pTab->GPTDataOf(n);
      if (silencer.silence(); table->SaveGPTBackup(output)) {
        silencer.stop();
        Log::println("Backup of {} is saved to {}", n, output);
      } else {
        silencer.stop();
        Log::print("Failed to backup {}!", n);
        SPAWN_ERROR();
      }
    }

    return true;
  }

  /**
   * @brief Restore GPT tables from backup files.
   *
   * @return true if successful.
   */
  bool restoreGptTable() {
    if (!file_list.empty() && table_names.size() != file_list.size())
      throw Helper::Error("You must provide an file name(s) as long as the table name(s)").cmdlineError().withCode(EX_USAGE);

    for (size_t i = 0; i < table_names.size(); i++) {
      const auto &n = table_names[i];
      if (!pTab->hasTable(n)) continue;
      std::string input = file_list[i];
      if (!directory.empty()) input.insert(0, directory + '/');

      auto table = pTab->GPTDataOf(n);
      if (silencer.silence(); table->LoadGPTBackup(input)) {
        silencer.stop();
        Log::println("{} is loaded on {}. Writing and syncing...", input, n);
        if (silencer.silence(); !pTab->sync(n)) {
          silencer.stop();
          Log::print("Failed to save changes to {}!", n);
          SPAWN_ERROR();
        }
        silencer.stop();
      } else {
        silencer.stop();
        Log::print("Failed to write {} backup to {}!", input, n);
        SPAWN_ERROR();
      }
    }

    return true;
  }

public:
  /// @brief Default constructor.
  PLUGIN_SECTION GptPlugin() = default;
  /// @brief Default destructor.
  PLUGIN_SECTION ~GptPlugin() override = default;

  /**
   * @brief Load the plugin and register its subcommands.
   *
   * @param mainApp The main application instance.
   * @param mainFlags The global flags structure.
   * @return true if the plugin loaded successfully.
   */
  PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &mainApp, BasicFlags &mainFlags) override {
    flags = &mainFlags;
    Log::info("{}::onLoad() trigger. Initializing...", PLUGIN);
    mainCmd = mainApp.addSubcommand("gpt", "GPT operations.")->requiresSubcommand();
    mainCmd->addFlag("-v,--version", nullptr, "View version of plugin.")
        ->superior()
        ->callback(Helper::CMDLine::Callbacks::ViewPluginVersion(PLUGIN, PLUGIN_VERSION));

    subCmdFirst = mainCmd
                      ->addSubcommand("read-table",
                                      "Send a command to the Linux kernel to read the specified partition table(s) (using ioctl())")
                      ->footer("Use read-all as table(s) for re-reading all partition tables.");
    subCmdFirst->addOption("table(s)", table_names, "Names of the partition tables to read")->required();

    subCmdSecond = mainCmd->addSubcommand("backup-table", "Backup specified GPT partition table(s). This is a backup of all data.")
                       ->footer("It backs up the protective MBR, main and second GPT header and partition entries. Use read-all as "
                                "table(s) for re-reading all partition tables.");
    subCmdSecond->addOption("table(s)", table_names, "Names of the partition tables to backup")->required();
    subCmdSecond->addOption("output(s)", file_list, "Names of the backup files to backup");
    subCmdSecond->addOption("-o,--output-directory", directory, "Directory to save GPT backup(s).");

    subCmdThird =
        mainCmd
            ->addSubcommand(
                "restore-table",
                "Restore specified GPT partition table(s). This is a backup of all data. It only backs up the GPT structure.")
            ->footer("Use read-all as table(s) for re-reading all partition tables.");
    subCmdThird->addOption("table(s)", table_names, "Names of the partition tables to restore")->required();
    subCmdThird->addOption("file(s)", file_list, "Names of the backup files to restore")->required();
    subCmdThird->addOption("-F,--file-directory", directory, "Directory to restore GPT backup(s).");

    return true;
  }

  /**
   * @brief Unload the plugin and clean up resources.
   *
   * @return true if the plugin unloaded successfully.
   */
  PLUGIN_SECTION bool onUnload() override {
    Log::info("{}::onUnload() trigger. Bye!", PLUGIN);
    mainCmd = nullptr;
    return true;
  }

  /**
   * @brief Check if the plugin's subcommand was used.
   *
   * @return true if the subcommand was used.
   */
  PLUGIN_SECTION bool used() override { return mainCmd->isUsed(); }

  /**
   * @brief Run the GPT operation.
   *
   * @return true if the operation succeeded.
   */
  PLUGIN_SECTION bool run() override {
    pTab = GET_PARTITION_TABLE_DATA_PTR();

    for (const auto &n : table_names) {
      if (n == "read-all") break;
      if (!pTab->hasTable(n)) {
        Log::print("{} table is not exists.", n);
        SPAWN_ERROR();
      }
    }

    if (subCmdFirst->isUsed())
      return reReadTables();
    else if (subCmdSecond->isUsed())
      return backupGptTable();
    else if (subCmdThird->isUsed())
      return restoreGptTable();

    return false;
  }

  /**
   * @brief Get the plugin name.
   *
   * @return std::string The plugin name.
   */
  PLUGIN_SECTION std::string getName() override { return PLUGIN; }

  /**
   * @brief Get the plugin version.
   *
   * @return std::string The plugin version.
   */
  PLUGIN_SECTION std::string getVersion() override { return PLUGIN_VERSION; }
};

} // namespace PartitionManager

REGISTER_PLUGIN(PartitionManager, GptPlugin)
