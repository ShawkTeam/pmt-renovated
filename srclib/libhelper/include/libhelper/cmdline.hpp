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

#ifndef LIBHELPER_CMDLINE_HPP
#define LIBHELPER_CMDLINE_HPP

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <sstream>
#include <algorithm>
#include <optional>
#include <cctype>
#include <concepts>
#include <libhelper/error.hpp>
#include <libhelper/definations.hpp>
#include <libhelper/functions.hpp>
#include <sysexits.h>

/**
 * @namespace Helper::CMDLine
 * @brief Command line parsing library. Inspired by the CLI11 project.
 */
namespace Helper::CMDLine {

using Error = Helper::Error;

inline std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> tokens;
  std::stringstream ss(s);
  std::string token;
  while (std::getline(ss, token, delim))
    if (!token.empty()) tokens.push_back(token);
  return tokens;
}

template <typename T> inline std::string getTypeName() {
  using cleanType = std::decay_t<T>;
  if constexpr (std::is_same_v<cleanType, bool>) return "bool";
  if constexpr (std::is_same_v<cleanType, int>) return "int";
  if constexpr (std::is_same_v<cleanType, float>) return "float";
  if constexpr (std::is_same_v<cleanType, double>) return "double";
  if constexpr (std::is_same_v<cleanType, char *> || std::is_same_v<cleanType, const char *> ||
                std::is_same_v<cleanType, std::string> || std::is_same_v<cleanType, std::string_view>)
    return "string";
  if constexpr (IsContainer<cleanType>) return "list";

  return typeid(T).name();
}

inline std::string toUpper(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
  return s;
}

inline std::string indentLines(const std::string &s, const std::string &indent = "  ") {
  std::string result;
  std::istringstream ss(s);
  std::string line;
  while (std::getline(ss, line))
    result += indent + line + "\n";
  return result;
}

template <typename T> T from_string(const std::string &s);

template <> inline int from_string<int>(const std::string &s) { return std::stoi(s); }

template <> inline unsigned int from_string<unsigned int>(const std::string &s) { return std::stoi(s); }

template <> inline long from_string<long>(const std::string &s) { return std::stol(s); }

template <> inline long long from_string<long long>(const std::string &s) { return std::stoll(s); }

template <> inline unsigned long from_string<unsigned long>(const std::string &s) { return std::stoul(s); }

template <> inline unsigned long long from_string<unsigned long long>(const std::string &s) { return std::stoull(s); }

template <> inline double from_string<double>(const std::string &s) { return std::stod(s); }

template <> inline float from_string<float>(const std::string &s) { return std::stof(s); }

template <> inline std::string from_string<std::string>(const std::string &s) { return s; }

template <> inline std::filesystem::path from_string<std::filesystem::path>(const std::string &s) { return std::filesystem::path(s); }

template <> inline char from_string<char>(const std::string &s) {
  if (s.empty()) throw Error("Empty char option").cmdlineError().withCode(EX_DATAERR);
  return s[0];
}

template <> inline bool from_string<bool>(const std::string &s) {
  if (s == "true" || s == "1" || s == "yes") return true;
  if (s == "false" || s == "0" || s == "no") return false;
  throw Error("Invalid bool value: {}", s).cmdlineError().withCode(EX_DATAERR);
}

template <> inline std::vector<int> from_string<std::vector<int>>(const std::string &s) {
  std::vector<int> result;
  for (auto &tok : split(s, ','))
    result.push_back(std::stoi(tok));
  return result;
}

template <> inline std::vector<std::string> from_string<std::vector<std::string>>(const std::string &s) { return split(s, ','); }

struct OptionProperties {
  OptionProperties() = default;

  OptionProperties(OptionProperties &&other) = default;
  OptionProperties &operator=(OptionProperties &&other) = default;

  OptionProperties(const OptionProperties &other) = default;
  OptionProperties &operator=(const OptionProperties &other) = default;

  std::vector<std::string> valid_names;
  std::function<void(const std::string &)> setter;
  std::function<void()> default_setter;
  std::function<void()> callback;
  std::function<void(const std::string &)> checker;
  std::function<std::string(const std::string &)> transformer;
  std::string description;
  std::string option_typename;
  std::string default_value;
  char option_delimiter = ',';
  bool is_required = false;
  bool is_positional = false;
  bool is_flag = false;
  bool is_found = false;
  bool is_used_as_long = false;
  bool early = false;
  bool superior = false;

  bool operator==(const OptionProperties &other) const {
    return valid_names == other.valid_names && description == other.description && option_typename == other.option_typename &&
           default_value == other.default_value && option_delimiter == other.option_delimiter && is_required == other.is_required &&
           is_flag == other.is_flag && is_found == other.is_found && is_used_as_long == other.is_used_as_long &&
           early == other.early && superior == other.superior;
  }

  bool operator!=(const OptionProperties &other) const { return !(*this == other); }
};

typedef enum { WITH_LONG_NAME, WITH_SHORT_NAME } UsageType;

/**
 * @namespace Transformers
 * @brief A collection of transformers for converting strings to other types.
 */
namespace Transformers {

/// @brief Converts a string to a size value. Inspired by the CLI11 project.
inline std::function<std::string(const std::string &)> AsSizeValue(bool use_si = false) {
  return [use_si](const std::string &s) -> std::string {
    if (s.empty()) throw std::invalid_argument("Empty size value");

    size_t i = 0;
    while (i < s.size() && (std::isdigit(s[i]) || s[i] == '.'))
      ++i;

    double num = std::stod(s.substr(0, i));
    std::string suffix = s.substr(i);
    suffix.erase(0, suffix.find_first_not_of(' '));
    for (auto &c : suffix)
      c = std::toupper(c);

    uint64_t multiplier = 1;
    if (use_si) {
      // 1K = 1000, 1M = 1000000, ...
      if (suffix == "K" || suffix == "KB")
        multiplier = 1000ULL;
      else if (suffix == "M" || suffix == "MB")
        multiplier = 1000ULL * 1000;
      else if (suffix == "G" || suffix == "GB")
        multiplier = 1000ULL * 1000 * 1000;
      else if (suffix == "T" || suffix == "TB")
        multiplier = 1000ULL * 1000 * 1000 * 1000;
    } else {
      // 1K = 1024, 1M = 1048576, ... (binary)
      if (suffix == "K" || suffix == "KIB")
        multiplier = 1024ULL;
      else if (suffix == "M" || suffix == "MIB")
        multiplier = 1024ULL * 1024;
      else if (suffix == "G" || suffix == "GIB")
        multiplier = 1024ULL * 1024 * 1024;
      else if (suffix == "T" || suffix == "TIB")
        multiplier = 1024ULL * 1024 * 1024 * 1024;
      else if (suffix == "KB")
        multiplier = 1024ULL; // tolerant
      else if (suffix == "MB")
        multiplier = 1024ULL * 1024;
      else if (suffix == "GB")
        multiplier = 1024ULL * 1024 * 1024;
    }

    uint64_t result = static_cast<uint64_t>(num * multiplier);
    return std::to_string(result);
  };
}

} // namespace Transformers

/**
 * @namespace Checkers
 * @brief A collection of checkers for validating strings.
 */
namespace Checkers {

inline std::function<void(const std::string &)> Exists() {
  return [](const std::string &s) {
    if (!Helper::isExists(s)) throw Error("{}: Entry is not exists.", s).withCode(EX_USAGE);
  };
}

inline std::function<void(const std::string &)> ExistingDirectory() {
  return [](const std::string &s) {
    if (!Helper::directoryIsExists(s)) throw Error("{}: Directory is not exists.", s).withCode(EX_USAGE);
  };
}

inline std::function<void(const std::string &)> ExistingFile() {
  return [](const std::string &s) {
    if (!Helper::fileIsExists(s)) throw Error("{}: File is not exists.", s).withCode(EX_USAGE);
  };
}

template <typename T> inline std::function<void(const std::string &)> BufferSizeCheck(T min, T max) {
  return [&](const std::string &s) {
    try {
      uint64_t size = from_string<uint64_t>(s);
      if (size < min || size > max) throw Error("{}: Buffer size is out of range.", s).cmdlineError().withCode(EX_USAGE);
    } catch (...) {
      throw Error("Invalid buffer size format").cmdlineError().withCode(EX_USAGE);
    }
  };
}

} // namespace Checkers

/**
 * @namespace Callbacks
 * @brief A collection of callbacks for handling option values, etc.
 */
namespace Callbacks {

inline std::function<void()> ViewPluginVersion(const std::string_view name, const std::string_view &ver, bool do_exit = true) {
  return [&]() {
    Out::println("{} v{}", name, ver);
    if (do_exit) std::exit(0);
  };
}

} // namespace Callbacks

class Option {
public:
  std::unique_ptr<OptionProperties> properties;

  Option() : properties(std::make_unique<OptionProperties>()) {}
  ~Option() = default;

  Option(const Option &) = delete;
  Option &operator=(const Option &) = delete;

  Option(Option &&other) noexcept = default;
  Option &operator=(Option &&other) noexcept = default;

  template <typename T>
  explicit Option(const std::string &spec, T &dest, const std::string &desc = "") : properties(std::make_unique<OptionProperties>()) {
    properties->valid_names = split(spec, ',');

    if (!properties->valid_names[0].starts_with("-")) {
      if (properties->valid_names.size() > 1)
        throw Error("A maximum of one positional option name can be specified.").cmdlineError().withCode(EX_CONFIG);

      properties->is_positional = true;
    } else {
      if (properties->valid_names.size() > 2)
        throw Error("A maximum of two separate option/flag names can be specified.").cmdlineError().withCode(EX_CONFIG);
    }

    properties->description = desc;
    properties->setter = [&dest](const std::string &raw) { dest = from_string<T>(raw); };
    properties->option_typename = getTypeName<T>();
  }

  explicit Option(const std::string &spec, std::nullptr_t, const std::string &desc = "")
      : properties(std::make_unique<OptionProperties>()) {
    properties->valid_names = split(spec, ',');

    if (!properties->valid_names[0].starts_with("-")) {
      if (properties->valid_names.size() > 1)
        throw Error("A maximum of one positional option name can be specified.").cmdlineError().withCode(EX_CONFIG);

      properties->is_positional = true;
    } else {
      if (properties->valid_names.size() > 2)
        throw Error("A maximum of two separate option/flag names can be specified.").cmdlineError().withCode(EX_CONFIG);
    }

    properties->description = desc;
    properties->setter = [](const std::string &raw) {};
    properties->option_typename = getTypeName<bool>();
  }

  void doToDoList(std::string raw) {
    if (properties->transformer) raw = properties->transformer(raw);
    if (properties->checker) properties->checker(raw);
    if (properties->setter) properties->setter(raw);
    properties->is_found = true;
    if (properties->callback) properties->callback();
  }

  Option *required(bool v = true) {
    properties->is_required = v;
    return this;
  }

  void setRequired(bool v = true) { properties->is_required = v; }

  Option *superior(bool v = true) {
    properties->superior = v;
    return this;
  }

  void setSuperior(bool v = true) { properties->superior = v; }

  Option *callback(std::function<void()> func) {
    properties->callback = std::move(func);
    return this;
  }

  void setCallback(std::function<void()> func) { properties->callback = std::move(func); }

  const std::function<void()> &getCallback() const { return properties->callback; }

  Option *check(std::function<void(const std::string &)> func) {
    properties->checker = std::move(func);
    return this;
  }

  void setChecker(std::function<void(const std::string &)> func) { properties->checker = std::move(func); }

  Option *transform(std::function<std::string(const std::string &)> func) {
    properties->transformer = std::move(func);
    return this;
  }

  void setTransform(std::function<std::string(const std::string &)> func) { properties->transformer = std::move(func); }

  const std::function<std::string(const std::string &)> &getTransform() const { return properties->transformer; }

  template <typename T> Option *defaultValue(T &&v) {
    if constexpr (std::is_same_v<std::decay_t<T>, const char *> || std::is_same_v<std::decay_t<T>, char *>)
      properties->default_value = v;
    else
      properties->default_value = std::to_string(v);
    properties->default_setter = [this]() { properties->setter(properties->default_value); };
    return this;
  }

  template <typename T> void setDefaultValue(T &&v) {
    if constexpr (std::is_same_v<std::decay_t<T>, const char *> || std::is_same_v<std::decay_t<T>, char *>)
      properties->default_value = v;
    else
      properties->default_value = std::to_string(v);
    properties->default_setter = [this]() { properties->setter(properties->default_value); };
  }

  const std::string &getDefaultValue() const { return properties->default_value; }

  Option *early(bool v = true) {
    properties->early = v;
    return this;
  }

  void setEarly(bool v = true) { properties->early = v; }

  Option *delimiter(char delim) {
    properties->option_delimiter = delim;
    return this;
  }

  void setDelimiter(char delim) { properties->option_delimiter = delim; }

  char getDelimiter() const { return properties->option_delimiter; }

  Option *isFlag(bool v = true) {
    if (properties->is_positional) throw Error("Options marked as positional cannot be flagged.").cmdlineError().withCode(EX_CONFIG);
    properties->is_flag = v;
    return this;
  }

  OptionProperties *getProperties() { return properties.get(); }

  const OptionProperties *getProperties() const { return properties.get(); }

  std::string shortName() const {
    for (auto &name : properties->valid_names)
      if (name.starts_with("-") && !name.starts_with("--")) return name;
    return {};
  }

  std::string longName() const {
    for (auto &name : properties->valid_names)
      if (name.starts_with("--")) return name;
    return {};
  }

  std::string positionalName() const {
    if (properties->is_positional) return properties->valid_names[0];
    return {};
  }

  UsageType getUsageType() const { return properties->is_used_as_long ? WITH_LONG_NAME : WITH_SHORT_NAME; }

  bool isUsed() const { return properties->is_found; }

  bool isSuperior() const { return properties->superior; }

  std::string getDescription() const { return properties->description; }

  bool operator==(const Option &other) const { return properties == other.properties; }
  bool operator!=(const Option &other) const { return !(*this == other); }

  explicit operator bool() const { return properties->is_found; }
};

class Subcommand {
  friend class App;

  std::vector<std::unique_ptr<Option>> options;
  bool is_found = false;

public:
  std::string name;
  std::string description;
  std::string _footer;

  Subcommand() = default;
  Subcommand(const std::string &name, const std::string desc = "") : name(name), description(desc) {}

  Subcommand(const Subcommand &) = delete;
  Subcommand &operator=(const Subcommand &) = delete;

  Subcommand(Subcommand &&other) = default;
  Subcommand &operator=(Subcommand &&other) = default;

  template <typename T> Option *addOption(const std::string &spec, T &dest, const std::string &desc = "") {
    auto arg = std::make_unique<Option>(spec, dest, desc);
    options.push_back(std::move(arg));
    return options.back().get();
  }

  void addOption(Option *orig) {
    if (orig == nullptr) throw Error("Input option pointer is null.").cmdlineError().withCode(EX_CONFIG);
    std::unique_ptr<Option> arg(orig);
    options.push_back(std::move(arg));
  }

  template <typename T> Option *addFlag(const std::string &spec, T &dest, const std::string &desc = "") {
    auto arg = std::make_unique<Option>(spec, dest, desc);
    arg->getProperties()->is_flag = true;
    options.push_back(std::move(arg));
    return options.back().get();
  }

  Option *addFlag(const std::string &spec, std::nullptr_t, const std::string &desc = "") {
    auto arg = std::make_unique<Option>(spec, nullptr, desc);
    arg->getProperties()->is_flag = true;
    options.push_back(std::move(arg));
    return options.back().get();
  }

  void addFlag(Option *orig) {
    if (orig == nullptr) throw Error("Input option pointer is null.").cmdlineError().withCode(EX_CONFIG);
    orig->getProperties()->is_flag = true;
    std::unique_ptr<Option> arg(orig);
    options.push_back(std::move(arg));
  }

  Subcommand *footer(const std::string &s) {
    _footer = s;
    return this;
  }

  void setFooter(const std::string &s) { _footer = s; }

  bool containsSuperiorOption() const {
    for (const auto &arg : options)
      if (arg->getProperties()->superior) return true;
    return false;
  }

  bool anySuperiorIsUsed() const {
    for (const auto &arg : options)
      if (arg->getProperties()->superior && arg->isUsed()) return true;
    return false;
  }

  bool isUsed() const { return is_found; }

  std::string getDescription() const { return description; }
  std::string getFooter() const { return _footer; }

  std::vector<std::unique_ptr<Option>> &getOptions() { return options; }

  const std::vector<std::unique_ptr<Option>> &getOptions() const { return options; }

  bool operator==(const Subcommand &other) const {
    return name == other.name && description == other.description && _footer == other._footer && options == other.options;
  }

  bool operator!=(const Subcommand &other) const { return !(*this == other); }

  explicit operator bool() const { return is_found; }
};

class App {
  std::string name;
  std::string cmd_name;
  std::string description;
  std::string license_string;
  bool subcmd_required;
  bool auto_help_enabled;
  bool help_triggered = false;
  bool fallback = false;
  std::vector<std::unique_ptr<Subcommand>> subcommands;
  std::vector<std::unique_ptr<Option>> options;

  Option *findOption(const std::string &_name, const std::vector<std::unique_ptr<Option>> &arg_list) {
    for (const auto &arg : arg_list) {
      auto props = arg->getProperties();
      if (std::find(props->valid_names.begin(), props->valid_names.end(), _name) != props->valid_names.end()) {
        return arg.get();
      }
    }
    return nullptr;
  }

  Option *getPositionalOption(size_t index, const std::vector<std::unique_ptr<Option>> &arg_list) {
    size_t count = 0;
    for (const auto &arg : arg_list) {
      if (arg->getProperties()->is_positional) {
        if (count == index) {
          return arg.get();
        }
        count++;
      }
    }
    return nullptr;
  }

  void printAlignedOption(const std::string &left_part, const std::string &_description) {
    const size_t indent_width = 8;
    const size_t max_line_width = 75;

    std::cout << "  " << left_part << "\n";

    size_t start = 0;
    while (start < _description.length()) {
      size_t chunk_size = max_line_width - indent_width;

      if (start + chunk_size < _description.length()) {
        size_t space_pos = _description.rfind(' ', start + chunk_size);
        if (space_pos != std::string::npos && space_pos > start) {
          chunk_size = space_pos - start;
        }
      }

      std::cout << std::string(indent_width, ' ') << _description.substr(start, chunk_size) << "\n";

      start += chunk_size;
      if (start < _description.length() && _description[start] == ' ') {
        start++;
      }
    }

    if (_description.empty()) std::cout << "\n";
  }

public:
  App() = default;
  App(const std::string &name, const std::string &desc = "")
      : name(name), cmd_name(name), description(desc), subcmd_required(false), auto_help_enabled(true) {}

  App(App &&other) = default;
  App &operator=(App &&other) = default;

  App(const App &) = delete;
  App &operator=(const App &) = delete;

  void setAutoHelp(bool enabled) { auto_help_enabled = enabled; }

  template <typename T> Option *addOption(const std::string &spec, T &dest, const std::string &desc = "") {
    auto arg = std::make_unique<Option>(spec, dest, desc);
    options.push_back(std::move(arg));
    return options.back().get();
  }

  void addOption(Option *orig) {
    if (orig == nullptr) throw Error("Input option pointer is null.").cmdlineError().withCode(EX_CONFIG);
    std::unique_ptr<Option> arg(orig);
    options.push_back(std::move(arg));
  }

  template <typename T> Option *addFlag(const std::string &spec, T &dest, const std::string &desc = "") {
    auto arg = std::make_unique<Option>(spec, dest, desc);
    arg->getProperties()->is_flag = true;
    options.push_back(std::move(arg));
    return options.back().get();
  }

  Option *addFlag(const std::string &spec, std::nullptr_t, const std::string &desc = "") {
    auto arg = std::make_unique<Option>(spec, nullptr, desc);
    arg->getProperties()->is_flag = true;
    options.push_back(std::move(arg));
    return options.back().get();
  }

  void addFlag(Option *orig) {
    if (orig == nullptr) throw Error("Input option pointer is null.").cmdlineError().withCode(EX_CONFIG);
    orig->getProperties()->is_flag = true;
    std::unique_ptr<Option> arg(orig);
    options.push_back(std::move(arg));
  }

  Subcommand *addSubcommand(const std::string &spec, const std::string &desc = "") {
    auto cmd = std::make_unique<Subcommand>(spec, desc);
    subcommands.push_back(std::move(cmd));
    return subcommands.back().get();
  }

  void addSubcommand(Subcommand *orig) {
    if (orig == nullptr) throw Error("Input subcommand pointer is null.").cmdlineError().withCode(EX_CONFIG);
    std::unique_ptr<Subcommand> arg(orig);
    subcommands.push_back(std::move(arg));
  }

  Subcommand *getSubcommand(const std::string &_name) {
    for (auto &subcmd : subcommands) {
      if (subcmd->name == _name) {
        return subcmd.get();
      }
    }
    return nullptr;
  }

  std::optional<std::string> getUsedSubcommandName() {
    for (const auto &subcmd : subcommands) {
      if (subcmd->isUsed()) {
        return subcmd->name;
      }
    }
    return std::nullopt;
  }

  void setLicenseString(const std::string &s) { license_string = s; }

  void setFallback(bool v = true) { fallback = v; }

  void help(const Subcommand *subcmd = nullptr) {
    if (subcmd) {
      std::cout << "Usage: " << cmd_name << " " << subcmd->name;

      [[maybe_unused]] bool has_positional = false;
      for (const auto &arg : subcmd->options) {
        if (arg->getProperties()->is_positional) {
          std::cout << " [" << arg->getProperties()->valid_names[0] << "]";
          has_positional = true;
        }
      }
      std::cout << " [OPTIONS]\n\n";
      std::cout << "Description:\n  " << subcmd->description << "\n\n";
      if (!subcmd->getFooter().empty()) std::cout << indentLines(subcmd->getFooter()) << "\n\n";

      if (!subcmd->options.empty()) {
        std::cout << "Subcommand Options:\n";
        for (const auto &arg : subcmd->options) {
          std::string left_part = "";
          for (size_t i = 0; i < arg->getProperties()->valid_names.size(); ++i) {
            left_part += arg->getProperties()->valid_names[i];
            if (i + 1 < arg->getProperties()->valid_names.size()) left_part += ", ";
          }

          if (arg->getProperties()->is_required) left_part += " (required)";
          if (arg->getProperties()->superior) left_part += " (superior)";
          if (!arg->getProperties()->is_flag) left_part += " <" + toUpper(arg->getProperties()->option_typename) + ">";

          printAlignedOption(left_part, arg->getDescription());
        }
        std::cout << "\n";
      }

      std::cout << "See " << cmd_name << " -h or --help for global options.\n";
      if (!license_string.empty()) std::cout << license_string << std::endl;
    } else {
      std::cout << "Usage: " << cmd_name;
      if (!subcommands.empty()) std::cout << " [SUBCOMMAND]";
      if (!options.empty()) std::cout << " [OPTIONS]";
      std::cout << "\n\n" << description << "\n\n";

      if (!subcommands.empty()) {
        std::cout << "Subcommands:\n";
        for (const auto &sc : subcommands)
          printAlignedOption(sc->name, sc->getDescription());
        std::cout << "\n";
      }

      if (!options.empty()) {
        std::cout << "Global Options:\n";
        for (const auto &arg : options) {
          std::string left_part = "";
          for (size_t i = 0; i < arg->getProperties()->valid_names.size(); ++i) {
            left_part += arg->getProperties()->valid_names[i];
            if (i + 1 < arg->getProperties()->valid_names.size()) left_part += ", ";
          }

          if (arg->getProperties()->is_required) left_part += " (required)";
          if (arg->getProperties()->superior) left_part += " (superior)";
          if (!arg->getProperties()->is_flag) left_part += " <" + toUpper(arg->getProperties()->option_typename) + ">";

          printAlignedOption(left_part, arg->getDescription());
        }

        if (!license_string.empty()) std::cout << std::endl << license_string << std::endl;
      }
    }
  }

  void parse_earlies(int argc, char *argv[], bool no_unknown_arg_error = true) {
    for (auto &arg : options)
      if (arg->getProperties()->default_setter) arg->getProperties()->default_setter();

    for (size_t i = 0; i < argc; ++i) {
      const std::string &token = argv[i];

      if (token.starts_with("-")) {
        std::string arg_name = token;
        std::string arg_value = "";
        bool has_equals = false;

        size_t equals_pos = token.find('=');
        if (equals_pos != std::string::npos) {
          arg_name = token.substr(0, equals_pos);
          arg_value = token.substr(equals_pos + 1);
          has_equals = true;
        }

        Option *matched_arg = findOption(arg_name, this->options);
        if (!matched_arg) {
          if (!no_unknown_arg_error)
            throw Error("Unknown option: {}", arg_name).cmdlineError().withCode(EX_USAGE);
          else
            continue;
        }

        auto props = matched_arg->getProperties();
        if (!props->early) continue;

        if (props->is_flag) {
          matched_arg->doToDoList("true");
        } else {
          if (has_equals) {
            matched_arg->doToDoList(arg_value);
          } else {
            if (i + 1 < argc && !std::string_view(argv[i + 1]).starts_with("-")) {
              matched_arg->doToDoList(argv[i + 1]);
              i++;
            } else
              throw Error("Missing early option value for: {}", arg_name).cmdlineError().withCode(EX_USAGE);
          }
        }

        continue;
      }
    }

    for (const auto &arg : options) {
      if (arg->getProperties()->is_required && arg->getProperties()->early && !arg->isUsed())
        throw Error("Missing required early option: {}", arg->getProperties()->valid_names[0]).cmdlineError().withCode(EX_USAGE);
    }
  }

  void parse(int argc, char *argv[]) {
    for (auto &arg : options)
      if (arg->getProperties()->default_setter) arg->getProperties()->default_setter();

    cmd_name = argv[0];
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
      args.push_back(argv[i]);

    Subcommand *active_subcommand = nullptr;
    size_t app_positional_index = 0;
    size_t subcmd_positional_index = 0;

    for (size_t i = 0; i < args.size(); ++i) {
      const std::string &token = args[i];

      if (auto_help_enabled && (token == "-h" || token == "--help")) {
        help(active_subcommand);
        std::exit(0);
      }

      if (!active_subcommand && !token.starts_with("-")) {
        Subcommand *subcmd = getSubcommand(token);
        if (subcmd) {
          active_subcommand = subcmd;
          active_subcommand->is_found = true;
          continue;
        }
      }

      if (token.starts_with("-")) {
        std::string arg_name = token;
        std::string arg_value = "";
        bool has_equals = false;

        size_t equals_pos = token.find('=');
        if (equals_pos != std::string::npos) {
          arg_name = token.substr(0, equals_pos);
          arg_value = token.substr(equals_pos + 1);
          has_equals = true;
        }

        Option *matched_arg = nullptr;

        if (active_subcommand) matched_arg = findOption(arg_name, active_subcommand->options);
        if (!matched_arg) matched_arg = findOption(arg_name, this->options);
        if (!matched_arg) throw Error("Unknown option: {}", arg_name).cmdlineError().withCode(EX_USAGE);

        auto props = matched_arg->getProperties();
        if (props->is_flag) {
          matched_arg->doToDoList("true");
        } else {
          if (has_equals)
            matched_arg->doToDoList(arg_value);
          else {
            if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
              matched_arg->doToDoList(args[i + 1]);
              i++;
            } else
              throw Error("Missing option value for: {}", arg_name).cmdlineError().withCode(EX_USAGE);
          }
        }
        continue;
      }

      Option *pos_arg = nullptr;

      if (active_subcommand) {
        pos_arg = getPositionalOption(subcmd_positional_index, active_subcommand->options);
        if (pos_arg) subcmd_positional_index++;
      }

      if (!pos_arg) {
        pos_arg = getPositionalOption(app_positional_index, this->options);
        if (pos_arg) app_positional_index++;
      }

      if (pos_arg)
        pos_arg->doToDoList(token);
      else
        throw Error("Unexpected option/subcommand: {}", token).cmdlineError().withCode(EX_USAGE);
    }

    for (const auto &arg : options) {
      if (arg->getProperties()->is_required && !arg->isUsed())
        throw Error("Missing required option: {}", arg->getProperties()->valid_names[0]).cmdlineError().withCode(EX_USAGE);
    }
    if (active_subcommand) {
      for (const auto &arg : active_subcommand->options) {
        if (arg->getProperties()->is_required && !arg->isUsed() && !active_subcommand->anySuperiorIsUsed())
          throw Error("Missing required subcommand option: {}", arg->getProperties()->valid_names[0])
              .cmdlineError()
              .withCode(EX_USAGE);
      }
    }
  }

  bool containsSuperiorOption() const {
    for (const auto &arg : options)
      if (arg->getProperties()->superior) return true;
    for (const auto &cmd : subcommands)
      if (cmd->containsSuperiorOption()) return true;
    return false;
  }

  bool anySuperiorIsUsed() const {
    for (const auto &arg : options)
      if (arg->getProperties()->superior && arg->isUsed()) return true;
    for (const auto &cmd : subcommands)
      if (cmd->anySuperiorIsUsed()) return true;
    return false;
  }
};

} // namespace Helper::CMDLine

#endif // LIBHELPER_CMDLINE_HPP
