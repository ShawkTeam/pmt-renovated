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
 * @file Plugin.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Plugin header file.
 */

#ifndef PARTITION_MANAGER_PLUGIN_HPP
#define PARTITION_MANAGER_PLUGIN_HPP

#if __cplusplus < 202002L
#error "Partition Manager Tool's plugin system is requires C++20 or higher C++ standarts."
#endif

#define PM "PluginManager"
#define PM_VERSION "1.1" ///< PluginManager version.

// clang-format off
#define Flags          (*flags) ///< Flags pointer
#define Tables         (*(Flags.partitionTables.first)) ///< Partition tables pointer
#define DEFAULT_PLUGIN_CONSTRUCTOR : flags(nullptr) {} ///< Recommended plugin constructor boddy. @deprecated Don't use this!
#define GET_PARTITION_TABLE_DATA_PTR Flags.partitionTables.first.get
#define GET_DYNAMIC_TABLE_DATA_PTR Flags.partitionTables.second.get

#define PLUGIN_END_WITH_RENDERER(r, m) \
  if (r) { r->start(); } \
  m.startAll(); \
  m.getResults(); \
  if (r) { r->stop(); } \
  return m.finalize()

#ifdef BUILTIN_PLUGINS
#define PLUGIN_SECTION __attribute__((section(".builtin_modules"))) ///< Custom section macro for builtin plugins.

/// @brief Register a builtin plugin.
#define REGISTER_PLUGIN(__namespace, __class)                                               \
  struct __class##_AutoRegister {                                                           \
    __class##_AutoRegister() {                                                              \
      PartitionManager::BuiltinPluginRegistry<PartitionManager::BasicPlugin>::getInstance() \
        .registerPlugin([]() {                                                              \
          return new __namespace::__class();                                                \
        });                                                                                 \
    }                                                                                       \
  };                                                                                        \
  static __class##_AutoRegister _reg;
#else
#define PLUGIN_SECTION __attribute__((section(".pmt_module"))) ///< Custom section macro for plugins.

/// @brief Register a plugin.
#define REGISTER_PLUGIN(__namespace, __class) \
  extern "C" PLUGIN_SECTION PartitionManager::BasicPlugin *create_plugin() { return new __namespace::__class(); }
#endif // #ifdef BUILTIN_PLUGINS
// clang-format on

#include <functional>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>
#include <format>
#include <dlfcn.h>
#include <libhelper/logging.hpp>
#include <libhelper/cmdline.hpp>
#include <libpartition_map/lib.hpp>

namespace PartitionManager {

/**
 * @brief Basic plugin class. The main class of all plugins.
 * @note All plugins should be similar to the ones here. Implementing the custom section is important.
 */
class BasicPlugin {
public:
  /**
   * CLI::App *cmd = nullptr;
   * BasicFlags *flags = nullptr;
   * std::string logPath;
   */

  PLUGIN_SECTION BasicPlugin() = default;
  virtual PLUGIN_SECTION ~BasicPlugin() = default;

  /// @brief Called when the plugin is loaded.
  virtual PLUGIN_SECTION bool onLoad(Helper::CMDLine::App &, const std::string &, BasicFlags &) = 0;
  /// @brief Called when the plugin is unloaded.
  virtual PLUGIN_SECTION bool onUnload() = 0;
  /// @brief Returns true if the plugin is used in command line.
  virtual PLUGIN_SECTION bool used() = 0;
  /// @brief Called when the plugin is run.
  virtual PLUGIN_SECTION bool run() = 0;

  /// @brief Get plugin name.
  virtual PLUGIN_SECTION std::string getName() = 0;
  /// @brief Get plugin version.
  virtual PLUGIN_SECTION std::string getVersion() = 0;
}; // class BasicPlugin

using PluginError = Helper::Error;

/// @brief Concept for minimum plugin class.
template <typename Class>
concept minimumPluginClass = requires {
  requires std::is_class_v<Class>;
  requires std::is_base_of_v<BasicPlugin, Class>;
}; // concept minimumPluginClass

/**
 * @brief Builtin plugin registry.
 *
 * @code
 * BuiltinPluginRegistry<BasicPlugin>::getInstance().registerPlugin([]() {
 *   return new MyPlugin();
 * });
 * @endcode
 *
 * @tparam PluginClass Plugin class.
 */
template <typename PluginClass = BasicPlugin>
  requires minimumPluginClass<PluginClass>
class BuiltinPluginRegistry {
public:
  using Factory = std::function<PluginClass *()>; ///< Plugin factory.

  /// @brief Get the singleton instance.
  static BuiltinPluginRegistry &getInstance() {
    static BuiltinPluginRegistry instance;
    return instance;
  }

  /// @brief Register a plugin.
  void registerPlugin(Factory function) { factories.push_back(function); }

  /// @brief Get registered plugins.
  const std::vector<Factory> &getPlugins() const { return factories; }

private:
  std::vector<Factory> factories;
}; // class BuiltinPluginRegistry

/**
 * @brief Plugin manager.
 *
 * @tparam BasePluginClass Base plugin class.
 * @see PartitionManager::BasicPlugin
 * @see PartitionManager::BuiltinPluginRegistry
 */
template <typename BasePluginClass>
  requires minimumPluginClass<BasePluginClass>
class PluginManager {
  using Creator = BasePluginClass *(*)();
  struct Plugin {
    std::string name;
    void *handle = nullptr;
    std::unique_ptr<BasePluginClass> instance;
  };

  std::vector<std::unique_ptr<BasePluginClass>> builtinPlugins;
  std::vector<Plugin> plugins;
  std::string logPath;
  Helper::CMDLine::App &mainApp;
  BasicFlags &mainFlags;

public:
  PluginManager() = delete; /// @brief Deleted constructor.

  /// @brief Main constructor.
  explicit PluginManager(Helper::CMDLine::App &cmd, std::string logpath, BasicFlags &flags)
      : logPath(std::move(logpath)), mainApp(cmd), mainFlags(flags) {}

  /// @brief Destructor.
  ~PluginManager() {
    LOGN(PM, INFO) << "Destructing all loaded plugins." << std::endl;

    for (auto &plugin : plugins) {
      plugin.instance->onUnload();
      plugin.instance.reset();
    }
    for (auto &plugin : builtinPlugins)
      plugin->onUnload();
  }

  /// @brief Load built-in plugins.
  bool loadBuiltinPlugins() {
    LOGN(PM, INFO) << "Loading built-in plugins." << std::endl;
    for (auto &plugin : BuiltinPluginRegistry<BasePluginClass>::getInstance().getPlugins()) {
      auto pluginHandle = plugin();
      LOGN(PM, INFO) << "Loading built-in plugin: " << pluginHandle->getName() << std::endl;
      if (!pluginHandle->onLoad(mainApp, logPath, mainFlags)) return false;
      builtinPlugins.emplace_back(std::move(pluginHandle));
    }

    return true;
  }

  /// @brief Load external plugin.
  bool loadPlugin(const std::string &pluginPath) {
    LOGN(PM, INFO) << "Loading external plugin: " << pluginPath << std::endl;

    void *handle = dlopen(pluginPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle) throw PluginError("dlopen failed: {}: {}", pluginPath, dlerror());

    auto create = (Creator)(dlsym(handle, "create_plugin"));
    if (!create) {
      dlclose(handle);
      throw PluginError("dlsym failed: {}: create_plugin: {}", pluginPath, dlerror());
    }

    auto plugin = std::unique_ptr<BasePluginClass>(create());
    if (alreadyExists(plugin->getName())) {
      LOGN(PM, ERROR) << plugin->getName() << " already exists!" << std::endl;
      return false;
    }
    plugin->onLoad(mainApp, logPath, mainFlags);

    LOGN(PM, INFO) << "Loaded external plugin: " << pluginPath << std::endl;
    plugins.push_back({plugin->getName(), handle, std::move(plugin)});

    return true;
  }

  /// @brief Run a plugin.
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

  /// @brief Run a plugin that is used in command line.
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

  /// @brief Check if a plugin is already loaded.
  bool alreadyExists(const std::string &name) const {
    LOGN(PM, INFO) << "Checking " << std::quoted(name) << " named plugin is exists or not." << std::endl;
    for (auto &plugin : plugins)
      if (plugin.name == name) return true;
    for (auto &plugin : builtinPlugins)
      if (plugin->getName() == name) return true;

    LOGN(PM, INFO) << std::quoted(name) << " named plugin is not exists." << std::endl;
    return false;
  }

  /// @note Returns @c alreadyExists().
  bool exists(const std::string &name) const { return alreadyExists(name); }

  /// @brief Returns the name of the used plugin.
  std::string getUsed() const {
    for (auto &plugin : plugins)
      if (plugin.instance->used()) return plugin.instance->getName();
    for (auto &plugin : builtinPlugins)
      if (plugin->used()) return plugin->getName();

    LOGN(PM, INFO) << "Don't used any used main command...." << std::endl;
    return {};
  }

  /// @brief Get plugin object.
  std::optional<std::reference_wrapper<BasePluginClass>> getPlugin(const std::string &name) {
    LOGN(PM, INFO) << "Considering plugin structure: " << name << std::endl;

    if (!exists(name)) return std::nullopt;
    for (auto &plugin : plugins)
      if (plugin.instance->getName() == name) return std::ref(*plugin.instance);
    for (auto &plugin : builtinPlugins)
      if (plugin->getName() == name) return std::ref(*plugin);

    return std::nullopt;
  }

  /// @brief Get plugin object (as const).
  std::optional<std::reference_wrapper<BasePluginClass>> getPlugin(const std::string &name) const {
    LOGN(PM, INFO) << "Considering plugin structure (as const): " << name << std::endl;

    if (!exists(name)) return std::nullopt;
    for (auto &plugin : plugins)
      if (plugin.instance->getName() == name) return std::cref(*plugin.instance);
    for (auto &plugin : builtinPlugins)
      if (plugin->getName() == name) return std::cref(*plugin);

    return std::nullopt;
  }

  /// @brief Get all loaded plugins.
  std::vector<Plugin> &getPlugins() { return plugins; }
  /// @brief Get all loaded plugins (as const).
  std::vector<Plugin> &getPlugins() const { return plugins; }

  /// @brief Get all loaded built-in plugins.
  std::vector<std::reference_wrapper<BasePluginClass>> getBuiltinPlugins() {
    std::vector<std::reference_wrapper<BasePluginClass>> refs;
    for (auto &plugin : builtinPlugins)
      refs.push_back(std::ref(*plugin));

    return refs;
  }

  /// @brief Get all loaded built-in plugins (as const).
  std::vector<std::reference_wrapper<BasePluginClass>> getBuiltinPlugins() const {
    std::vector<std::reference_wrapper<BasePluginClass>> crefs;
    for (auto &plugin : builtinPlugins)
      crefs.push_back(std::cref(*plugin));

    return crefs;
  }

  /// @brief Returns the version of @c PluginManager.
  std::string getVersion() const { return PM_VERSION; }

  /**
   * @name @c PluginManagers's operators.
   * @brief Operators of @c PluginManager class.
   * @{
   */

  /// @brief Returns the plugin object.
  std::optional<std::reference_wrapper<BasePluginClass>> operator()(const std::string &name) { return getPlugin(name); }

  /// @brief Returns the plugin object (as-const).
  std::optional<std::reference_wrapper<BasePluginClass>> operator()(const std::string &name) const { return getPlugin(name); }

  bool operator()() { return runUsed(); } ///< Returns the result of @c runUsed().

  /// @brief Returns all loaded plugins.
  std::vector<std::reference_wrapper<BasePluginClass>> operator*() {
    std::vector<std::reference_wrapper<BasePluginClass>> all_refs;
    auto builtins = getBuiltinPlugins();

    all_refs.reserve(builtins.size() + plugins.size());
    std::ranges::for_each(getPlugins(), [&](Plugin plugin) { all_refs.push_back(std::ref(*plugin.instance)); });
    all_refs.insert(all_refs.end(), builtins.begin(), builtins.end());

    return all_refs;
  }

  /// @brief Returns all loaded plugins (as const).
  std::vector<std::reference_wrapper<BasePluginClass>> operator*() const {
    std::vector<std::reference_wrapper<BasePluginClass>> all_refs;
    auto builtins = getBuiltinPlugins();

    all_refs.reserve(builtins.size() + plugins.size());
    std::ranges::for_each(getPlugins(), [&](const Plugin plugin) { all_refs.push_back(std::cref(*plugin.instance)); });
    all_refs.insert(all_refs.end(), builtins.begin(), builtins.end());

    return all_refs;
  }

  /** @} */

}; // class PluginManager

/// @brief A helper class that simplifies result generation in asynchronous processing plugins.
class AsyncResult_t {
public:
  std::string message;          ///< Message.
  std::string &first = message; ///< std::pair imitation.
  bool result = true;           ///< True = success, false = error.
  bool &second = result;        ///< std::pair imitation.

  /// @brief Default constructor.
  AsyncResult_t() = default;
  /// @brief Copy constructor.
  AsyncResult_t(const AsyncResult_t &other) = default;
  /// @brief Move constructor.
  AsyncResult_t(AsyncResult_t &&other) noexcept : message(std::move(other.message)), result(other.result) {}

  /// @brief Returns the message.
  std::string getMessage() const { return message; }
  /// @brief Returns the result.
  bool getResult() const { return result; }
  /// @brief Returns true if the result is an error.
  bool isError() const { return !result; }
  /// @brief Returns true if the result is a success.
  bool isSuccess() const { return result; }

  /// @brief Returns an error result.
  template <typename... Args> static AsyncResult_t Error(std::format_string<Args...> fmt, Args &&...args) {
    AsyncResult_t result;
    result.message = std::format(fmt, std::forward<Args>(args)...);
    result.result = false;
    return result;
  }

  /// @brief Returns an error result.
  static AsyncResult_t Error(const std::string &message = "") {
    AsyncResult_t result;
    result.message = message;
    result.result = false;
    return result;
  }

  /// @brief Returns a success result.
  template <typename... Args> static AsyncResult_t Success(std::format_string<Args...> fmt, Args &&...args) {
    AsyncResult_t result;
    result.message = std::format(fmt, std::forward<Args>(args)...);
    result.result = true;
    return result;
  }

  /// @brief Returns a success result.
  static AsyncResult_t Success(const std::string &message = "") {
    AsyncResult_t result;
    result.message = message;
    result.result = true;
    return result;
  }

  /// @brief Move assignment operator.
  AsyncResult_t &operator=(AsyncResult_t &&other) noexcept {
    message = std::move(other.message);
    result = other.result;
    return *this;
  }

  /// @brief Returns a pair of message and result.
  std::pair<std::string, bool> operator()() const { return std::make_pair(message, result); }
}; // class AsyncResult_t

using BasicManager = PluginManager<BasicPlugin>;                       ///< Short for @c PluginManager.
using BasicBuiltinPluginRegistry = BuiltinPluginRegistry<BasicPlugin>; ///< Short for @c BuiltinPluginRegistry.

/// @brief Splits the string if it contains the delimiter and checks for duplicate elements.
inline auto splitIfHasDelim = [](const std::string &s, const char delim, const bool checkForBadUsage) {
  if (s.find(delim) == std::string::npos) return std::vector<std::string>{};
  auto vec = Helper::CMDLine::split(s, delim);

  if (checkForBadUsage) {
    std::unordered_set<std::string> set;
    for (const auto &str : vec) {
      if (set.contains(str)) throw Error("Duplicate element in your inputs!").cmdlineError().withCode(EX_USAGE);
      set.insert(str);
    }
  }

  return vec;
};

/// @brief Process vectors with input strings. Use for [flag(s)]-[other flag(s)] situtations.
inline auto processCommandLine = [](std::vector<std::string> &vec1, std::vector<std::string> &vec2, const std::string &s1,
                                    const std::string &s2, const char delim, bool checkForBadUsage) {
  vec1 = splitIfHasDelim(s1, delim, checkForBadUsage);
  vec2 = splitIfHasDelim(s2, delim, checkForBadUsage);

  if (vec1.empty() && !s1.empty()) vec1.push_back(s1);
  if (vec2.empty() && !s2.empty()) vec2.push_back(s2);
};

/// @brief Setting ups correct buffer size for input entry.
inline auto setupBufferSize = [](uint64_t &size, const std::filesystem::path &entry, PartitionMap::BaseTableData *table) {
  if (table->hasPartition(entry.filename())) {
    const uint64_t psize = table->partition(entry.filename().string())->get().size();

    if (psize % size != 0) {
      Out::println("{}WARNING{}: Specified buffer size is invalid for {}! Using "
                   "different buffer size for {}.",
                   YELLOW, STYLE_RESET, entry.string(), entry.string());
      size = psize % 4096 == 0 ? 4096 : 1;
    }
  } else if (Helper::fileIsExists(entry)) {
    if (Helper::fileSize(entry) % size != 0) {
      Out::println("{}WARNING{}: Specified buffer size is invalid for {}! Using "
                   "different buffer size for {}.",
                   YELLOW, STYLE_RESET, entry.string(), entry.string());
      size = Helper::fileSize(entry) % 4096 == 0 ? 4096 : 1;
    }
  }
};

} // namespace PartitionManager

#endif // #ifndef PARTITION_MANAGER_PLUGIN_HPP
