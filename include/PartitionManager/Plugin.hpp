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

#ifndef PARTITION_MANAGER_PLUGIN_HPP
#define PARTITION_MANAGER_PLUGIN_HPP

#if __cplusplus < 202002L
#error "Partition Manager Tool's plugin system is requires C++20 or higher C++ standarts."
#endif

#define PM "PluginManager"
#define PM_VERSION "1.0"

#include <concepts>
#include <functional>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>
#include <dlfcn.h>
#include <libhelper/logging.hpp>
#include <libpartition_map/lib.hpp>
#include <CLI11.hpp>

namespace PartitionManager {

class BasicPlugin {
public:
  CLI::App *cmd = nullptr;
  FlagsBase flags;
  const char *logPath;

  virtual ~BasicPlugin() = default;

  virtual bool onLoad(CLI::App &mainApp, const std::string &logpath, FlagsBase &mainFlags) = 0;
  virtual bool onUnload() = 0;
  virtual bool used() = 0;
  virtual bool run() = 0;

  virtual std::string getName() = 0;
  virtual std::string getVersion() = 0;
}; // class BasicPlugin

using PluginError = Helper::Error;

template <typename __class>
concept minimumPluginClass = requires {
  requires std::is_class_v<__class>;
  requires std::is_base_of_v<BasicPlugin, __class>;
}; // concept minimumPluginClass

template <typename __class>
  requires minimumPluginClass<__class>
class BuiltinPluginRegistry {
public:
  using Factory = std::function<__class *()>;
  static BuiltinPluginRegistry &getInstance() {
    static BuiltinPluginRegistry instance;
    return instance;
  }

  void registerPlugin(Factory function) { factories.push_back(function); }

  const std::vector<Factory> &getPlugins() const { return factories; }

private:
  std::vector<Factory> factories;
};

template <typename __class>
  requires minimumPluginClass<__class>
class PluginManager {
public:
  explicit PluginManager(CLI::App &cmd, std::string logpath, FlagsBase &flags)
      : logPath(std::move(logpath)), mainApp(cmd), mainFlags(flags) {}

  ~PluginManager() {
    LOGN(PM, INFO) << "Unloading all loaded plugins." << std::endl;

    for (auto &plugin : plugins) {
      plugin.instance->onUnload();
      plugin.instance.reset();
    }
    for (auto &plugin : builtinPlugins)
      plugin->onUnload();
  }

  bool loadBuiltinPlugins() {
    LOGN(PM, INFO) << "Loading built-in plugins." << std::endl;
    for (auto &plugin : BuiltinPluginRegistry<__class>::getInstance().getPlugins()) {
      auto pluginHandle = plugin();
      LOGN(PM, INFO) << "Loading built-in plugin: " << pluginHandle->getName() << std::endl;
      if (!pluginHandle->onLoad(mainApp, logPath, mainFlags)) return false;
      builtinPlugins.emplace_back(std::move(pluginHandle));
    }

    return true;
  }

  bool loadPlugin(const std::string &pluginPath) {
    LOGN(PM, INFO) << "Loading external plugin: " << pluginPath << std::endl;

    void *handle = dlopen(pluginPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle) throw PluginError() << "dlopen failed: " << pluginPath << ": " << dlerror();

    auto create = (Creator)(dlsym(handle, "create_plugin"));
    if (!create) {
      dlclose(handle);
      throw PluginError() << "dlsym failed: " << pluginPath << ": create_plugin: " << dlerror();
    }

    auto plugin = std::unique_ptr<__class>(create());
    if (alreadyExists(plugin->getName())) {
      LOGN(PM, ERROR) << plugin->getName() << " already exists!" << std::endl;
      return false;
    }
    plugin->onLoad(mainApp, logPath, mainFlags);

    LOGN(PM, INFO) << "Loaded external plugin: " << pluginPath << std::endl;
    plugins.push_back({plugin->getName(), handle, std::move(plugin)});

    return true;
  }

  bool run(const std::string &name) {
    LOGN(PM, INFO) << "Running " << std::quoted(name) << " plugin if exists." << std::endl;
    for (auto &plugin : plugins) {
      if (plugin.name == name) return plugin.instance->run();
    }
    for (auto &plugin : builtinPlugins) {
      if (plugin->getName() == name) return plugin->run();
    }

    return false; // Not found any named plugin as 'name'.
  }

  bool runUsed() {
    LOGN(PM, INFO) << "Running caught subcommand in command line (if has)." << std::endl;
    for (auto &plugin : plugins) {
      if (plugin.instance->used()) return plugin.instance->run();
    }
    for (auto &plugin : builtinPlugins) {
      if (plugin->used()) return plugin->run();
    }

    return false; // Not used any plugin on command line.
  }

  bool alreadyExists(const std::string &name) {
    LOGN(PM, INFO) << "Checking " << std::quoted(name) << " named plugin is exists or not." << std::endl;
    for (auto &plugin : plugins)
      if (plugin.name == name) return true;
    for (auto &plugin : builtinPlugins)
      if (plugin->getName() == name) return true;

    LOGN(PM, INFO) << std::quoted(name) << " named plugin is not exists." << std::endl;
    return false;
  }

  std::string getVersion() const { return PM_VERSION; }

private:
  using Creator = __class *(*)();
  struct Plugin {
    std::string name;
    void *handle = nullptr;
    std::unique_ptr<__class> instance;
  };

  std::vector<std::unique_ptr<__class>> builtinPlugins;
  std::vector<Plugin> plugins;
  std::string logPath;
  CLI::App &mainApp;
  FlagsBase &mainFlags;
};

using BasicManager = PluginManager<BasicPlugin>;
using BasicBuiltinPluginRegistry = BuiltinPluginRegistry<BasicPlugin>;

// clang-format off
#define REGISTER_BUILTIN_PLUGIN(__namespace, __class)                                               \
        struct __class##_AutoRegister {                                                             \
            __class##_AutoRegister() {                                                              \
              PartitionManager::BuiltinPluginRegistry<PartitionManager::BasicPlugin>::getInstance() \
                .registerPlugin([]() {                                                              \
                    return new __namespace::__class();                                              \
                });                                                                                 \
            }                                                                                       \
        };                                                                                          \
        static __class##_AutoRegister _reg;

#define REGISTER_DYNAMIC_PLUGIN(__class) \
  extern "C" PartitionManager::BasicPlugin *create_plugin() { return new __class(); }
#define FLAGS (*flags)
#define TABLES (*FLAGS.partitionTables)
#define TABLES_REF *flags->partitionTables
// clang-format on

using resultPair = std::pair<std::string, bool>;

__attribute__((format(printf, 1, 2))) inline resultPair PairError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char str[1024];
  vsnprintf(str, sizeof(str), format, args);
  va_end(args);
  return {str, false};
}

__attribute__((format(printf, 1, 2))) inline resultPair PairSuccess(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char str[1024];
  vsnprintf(str, sizeof(str), format, args);
  va_end(args);
  return {str, true};
}

// If there is a delimiter in the string, CLI::detail::split (std::vector<std::string>) returns; if not, an empty vector is returned.
// And checks duplicate arguments.
inline auto splitIfHasDelim = [](const std::string &s, const char delim, const bool checkForBadUsage) {
  if (s.find(delim) == std::string::npos) return std::vector<std::string>{};
  auto vec = CLI::detail::split(s, delim);

  if (checkForBadUsage) {
    std::unordered_set<std::string> set;
    for (const auto &str : vec) {
      if (set.contains(str)) throw CLI::ValidationError("Duplicate element in your inputs!");
      set.insert(str);
    }
  }

  return vec;
};

// Process vectors with input strings. Use for [flag(s)]-[other flag(s)] situtations.
inline auto processCommandLine = [](std::vector<std::string> &vec1, std::vector<std::string> &vec2, const std::string &s1,
                                    const std::string &s2, const char delim, bool checkForBadUsage) {
  vec1 = splitIfHasDelim(s1, delim, checkForBadUsage);
  vec2 = splitIfHasDelim(s2, delim, checkForBadUsage);

  if (vec1.empty() && !s1.empty()) vec1.push_back(s1);
  if (vec2.empty() && !s2.empty()) vec2.push_back(s2);
};

// Setting ups correct buffer size for input entry.
inline auto setupBufferSize = [](uint64_t &size, const std::filesystem::path &entry, PartitionMap::Builder &builder) {
  if (builder.hasPartition(entry.filename())) {
    const uint64_t psize = builder.partition(entry.filename().string()).size();

    if (psize % size != 0) {
      Out::println("%sWARNING%s: Specified buffer size is invalid for %s! Using "
                   "different buffer size for %s.",
                   YELLOW, STYLE_RESET, entry.string().c_str(), entry.string().c_str());
      size = psize % 4096 == 0 ? 4096 : 1;
    }
  } else if (Helper::fileIsExists(entry)) {
    if (Helper::fileSize(entry) % size != 0) {
      Out::println("%sWARNING%s: Specified buffer size is invalid for %s! using "
                   "different buffer size for %s.",
                   YELLOW, STYLE_RESET, entry.string().c_str(), entry.string().c_str());
      size = Helper::fileSize(entry) % 4096 == 0 ? 4096 : 1;
    }
  }
};

} // namespace PartitionManager

#endif // #ifndef PARTITION_MANAGER_PLUGIN_HPP
