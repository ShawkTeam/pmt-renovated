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
#include <libhelper/lib.hpp>
#include <libpartition_map/lib.hpp>
#include <memory>
#include <string>
#include <vector>

#define PMT "libpmt"
#define PMTE "pmt"
#define PMTF "libpmt-function-manager"

namespace PartitionManager {
// All function classes must inherit from this class.
class basic_function {
public:
  CLI::App *cmd = nullptr;

  virtual bool init(CLI::App &_app) = 0;
  virtual bool run() = 0;

  [[nodiscard]] virtual bool isUsed() const = 0;
  [[nodiscard]] virtual const char *name() const = 0;

  virtual ~basic_function() = default;
};

// A class for function management.
class basic_function_manager final {
private:
  std::vector<std::unique_ptr<basic_function>> _functions;

public:
  void registerFunction(std::unique_ptr<basic_function> _func, CLI::App &_app);

  [[nodiscard]] bool isUsed(std::string name) const;
  [[nodiscard]] bool handleAll() const;
};

class basic_variables final {
public:
  basic_variables();
  ~basic_variables();

  PartitionMap::BuildMap *PartMap;

  std::string searchPath, logFile;
  bool onLogical;
  bool quietProcess;
  bool verboseMode;
  bool viewVersion;
  bool forceProcess;
};

using FunctionBase = basic_function;
using FunctionManager = basic_function_manager;
using VariableTable = basic_variables;
using Error = Helper::Error;

extern VariableTable *Variables;

int Main(int argc, char **argv);

// Print messages if not using quiet mode
__attribute__((format(printf, 1, 2))) void print(const char *format, ...);
__attribute__((format(printf, 1, 2))) void println(const char *format, ...);

// Format it input and return as std::string
__attribute__((format(printf, 1, 2))) std::string format(const char *format,
                                                         ...);

// If there is a delimiter in the string, CLI::detail::split returns; if not, an
// empty vector is returned. And checks duplicate arguments.
std::vector<std::string> splitIfHasDelim(const std::string &s, char delim,
                                         bool checkForBadUsage = false);

// Process vectors with input strings. Use for [flag(s)]-[other flag(s)]
// situations
void processCommandLine(std::vector<std::string> &vec1,
                        std::vector<std::string> &vec2, const std::string &s1,
                        const std::string &s2, char delim,
                        bool checkForBadUsage = false);

// Setting ups buffer size
void setupBufferSize(uint64_t &size, const std::string &entry);

std::string getLibVersion();

std::string getAppVersion(); // Not Android app version (an Android app is
                             // planned!), tells pmt version.
} // namespace PartitionManager

#endif // #ifndef LIBPMT_LIB_HPP