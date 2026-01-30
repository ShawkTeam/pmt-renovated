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

#ifndef LIBPMT_LIB_HPP
#define LIBPMT_LIB_HPP

#include <CLI/CLI11.hpp>
#include <concepts>
#include <format>
#include <libhelper/lib.hpp>
#include <libpartition_map/lib.hpp>
#include <memory>
#include <string>
#include <vector>

#define PMT "pmt-core"
#define PMTE "pmt"
#define PMTF "pmt-core--function-manager"

// Quick access to variables.
#define VARS (*Variables)

// Quick access to partition map.
#define PART_TABLES (*VARS.partitionTables)

#define OUT out

class OutUtil final {
private:
  Helper::garbageCollector collector;

public:
  FILE *standart = nullptr, *error = nullptr;

  OutUtil(); // Default constructor
  ~OutUtil() {}

  __printflike(2, 3) void print(const char *format, ...) const;
  __printflike(2, 3) void println(const char *format, ...) const;

  [[nodiscard]] FILE *getStandartPointer() const;
  [[nodiscard]] FILE *getErrorPointer() const;

  static FILE *makeFilePointer(FILE *real);
  static int writer(void *cookie, const char *buf, int size);
};

namespace PartitionManager {
int Main(int argc, char **argv);

// If there is a delimiter in the string, CLI::detail::split returns; if not, an
// empty vector is returned. And checks duplicate arguments.
std::vector<std::string> splitIfHasDelim(const std::string &s, char delim, bool checkForBadUsage = false);

// Process vectors with input strings. Use for [flag(s)]-[other flag(s)]
// situations
void processCommandLine(std::vector<std::string> &vec1, std::vector<std::string> &vec2, const std::string &s1, const std::string &s2,
                        char delim, bool checkForBadUsage = false);

// Setting ups buffer size
void setupBufferSize(uint64_t &size, const std::filesystem::path &entry);

PartitionMap::Partition_t &duplicateCheck(const std::string &name);

std::string getLibVersion();

std::string getAppVersion(); // Not Android app version (an Android app is
                             // planned!), tells pmt version.

extern OutUtil out;

enum basic_function_flags {
  NO_SU = 1,
  NO_MAP_CHECK = 2,
  ADB_SUFFICIENT = 3,
};

// All function classes must inherit from this class.
class basic_function {
public:
  CLI::App *cmd = nullptr;
  std::vector<int> flags = {};

  virtual bool init(CLI::App &_app) = 0;
  virtual bool run() = 0;

  [[nodiscard]] virtual bool isUsed() const = 0;
  [[nodiscard]] virtual const char *name() const = 0;

  virtual ~basic_function() = default;
};

// A class for function management.
template <typename __class>
  requires std::derived_from<__class, basic_function>
class basic_manager {
private:
  std::vector<std::unique_ptr<__class>> _functions;

public:
  void registerFunction(std::unique_ptr<__class> _func, CLI::App &_app) {
    LOGN(PMTF, INFO) << "registering: " << _func->name() << std::endl;
    for (const auto &f : _functions) {
      if (std::string(_func->name()) == std::string(f->name())) {
        LOGN(PMTF, INFO) << "Is already registered: " << _func->name() << ". Skipping." << std::endl;
        return;
      }
    }
    if (!_func->init(_app)) throw Helper::Error("Cannot init: %s", _func->name());
    _functions.push_back(std::move(_func));
    LOGN(PMTF, INFO) << _functions.back()->name() << " successfully registered." << std::endl;
  }

  [[nodiscard]] bool hasFlagOnUsedFunction(int flag) const {
    for (const auto &func : _functions) {
      if (func->isUsed()) {
        std::for_each(func->flags.begin(), func->flags.end(),
                      [&](const int x) { LOGN(PMTF, INFO) << "Used flag " << x << " on " << func->name() << std::endl; });
        return std::find(func->flags.begin(), func->flags.end(), flag) != func->flags.end();
      }
    }
    return false;
  }

  [[nodiscard]] bool isUsed(const std::string &name) const {
    if (_functions.empty()) return false;
    for (const auto &func : _functions) {
      if (func->name() == name) return func->isUsed();
    }
    return false;
  }

  [[nodiscard]] bool handleAll() const {
    LOGN(PMTF, INFO) << "running caught commands in command-line." << std::endl;
    for (const auto &func : _functions) {
      if (func->isUsed()) {
        LOGN(PMTF, INFO) << func->name() << " is calling because used in command-line." << std::endl;
        return func->run();
      }
    }

    LOGN(PMTF, INFO) << "not found any used function from command-line." << std::endl;
    OUT.println("Target progress is not specified. Specify a progress.");
    return false;
  }
};

class basic_variables final {
public:
  basic_variables();

  std::unique_ptr<PartitionMap::Builder> partitionTables;

  std::string logFile;
  std::set<std::string> extraTablePaths;
  bool onLogical;
  bool quietProcess;
  bool verboseMode;
  bool viewVersion;
  bool forceProcess;
  bool noWorkOnUsed;
};

using FunctionBase = basic_function;
using FunctionManager = basic_manager<FunctionBase>;
using FunctionFlags = basic_function_flags;
using VariableTable = basic_variables;
using Error = Helper::Error;

extern std::unique_ptr<VariableTable> Variables;
} // namespace PartitionManager

#endif // #ifndef LIBPMT_LIB_HPP
