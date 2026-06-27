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
 * @file logging.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief A logging solution that can be used throughout the program(s).
 */

#ifndef LIBHELPER_LOGGING_HPP
#define LIBHELPER_LOGGING_HPP

/**
 * @name Logging Macros
 * @brief Macro shortcuts for logging.
 * @deprecated Use new functions for doing this. See @c Log namespace.
 *
 * @code
 * // Basic logging.
 * LOG(INFO) << "Info message." << std::endl;
 * LOGF("file.log", INFO) << "Info message to file.log." << std::endl;
 * LOGN(HELPER, INFO) << "Info message with libhelper program name." << std::endl;
 * LOGNF(HELPER, "file.log", INFO) << "Info message with libhelper program name (to file.log)." << std::endl;
 *
 * // Logging with conditions.
 * // If the condition(s) are met, a log is written.
 * LOG_IF(INFO, 3 > 1) << "Info message." << std::endl;
 * LOGF_IF("file.log", INFO, 3 > 1 && 2 < 4) << "Info message to file.log." << std::endl;
 * LOGN_IF(HELPER, INFO, 3 < 1 || 4 < 5) << "Info message with libhelper program name." << std::endl;
 * LOGNF_IF(HELPER, "file.log", INFO, false) << "Info message with libhelper program name (to file.log)." << std::endl;
 * @endcode
 *
 * @{
 */
#define LOG(level) Helper::Logger(level, __func__, Helper::Logger::Properties::FILE.c_str(), __FILE__, __LINE__)
#define LOGF(file, level) Helper::Logger(level, __func__, file, __FILE__, __LINE__)

/// @warning This macro is unusable.
#define LOGN(name, level) Helper::Logger(level, __func__, Helper::Logger::Properties::FILE.c_str(), name, __FILE__, __LINE__)
/// @warning This macro is unusable.
#define LOGNF(name, file, level) Helper::Logger(level, __func__, file, name, __FILE__, __LINE__)

#define LOG_IF(level, condition)                                                                                                      \
  if (!(condition)) {                                                                                                                 \
  } else                                                                                                                              \
    Helper::Logger(level, __func__, Helper::Logger::Properties::FILE.c_str(), __FILE__, __LINE__)
#define LOGF_IF(file, level, condition)                                                                                               \
  if (!(condition)) {                                                                                                                 \
  } else                                                                                                                              \
    Helper::Logger(level, __func__, file, __FILE__, __LINE__)
/// @warning This macro is unusable.
#define LOGN_IF(name, level, condition)                                                                                               \
  if (!(condition)) {                                                                                                                 \
  } else                                                                                                                              \
    Helper::Logger(level, __func__, Helper::Logger::Properties::FILE.c_str(), name, __FILE__, __LINE__)
/// @warning This macro is unusable.
#define LOGNF_IF(name, file, level, condition)                                                                                        \
  if (!(condition)) {                                                                                                                 \
  } else                                                                                                                              \
    Helper::Logger(level, __func__, file, name, __FILE__, __LINE__)
/** @} */

#include <sstream>
#include <string>
#include <fstream>
#include <iomanip>
#include <source_location>

#include <libhelper/definations.hpp>

namespace std {

/**
 * @brief It is an implementation of @c std::quoted that returns @c std::string.
 * @param s Input string.
 * @return Quoted input string.
 */
template <typename _T> std::string quoted_string(_T &&s) {
  std::ostringstream oss;
  if constexpr (std::is_same_v<std::decay_t<_T>, std::filesystem::path> || std::is_same_v<std::decay_t<_T>, std::string>)
    oss << std::quoted(s.c_str());
  else if constexpr (std::is_same_v<std::decay_t<_T>, std::string_view>)
    oss << std::quoted(s.data());
  else
    oss << s;
  return oss.str();
}

} // namespace std

namespace Helper {

/// @brief Logging levels.
enum LogLevels {
  INFO = static_cast<int>('I'),
  WARNING = static_cast<int>('W'),
  ERROR = static_cast<int>('E'),
  ABORT = static_cast<int>('A')
};

/**
 * @brief Modern, functional logger class.
 * @note It is recommended to use ready-made macros to benefit from this class.
 * @see Helper::Logger::Properties
 */
class Logger {
  LogLevels level;
  std::ostringstream oss;
  std::string function, logFile, file;
  int line;

  std::string stripQuotes(const std::string &s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') return s.substr(1, s.size() - 2);
    return s;
  }

public:
  /**
   * @brief Transfers the contents of the old log file to the new one.
   *
   * @param oldLogFile Old log file.
   * @param newLogFile New log file.
   * @param remove Will the old log file be deleted?
   * @return Returns true if successful. Otherwise, false.
   * @note This function is called automatically when the log file path is changed.
   */
  static bool moveOldLogs(const std::filesystem::path &oldLogFile, const std::filesystem::path &newLogFile, bool remove = false) {
    try {
      if (!std::filesystem::exists(oldLogFile)) return false;
      std::filesystem::copy_file(oldLogFile, newLogFile, std::filesystem::copy_options::overwrite_existing);
      if (remove) std::filesystem::remove(oldLogFile);

      return true;
    } catch (...) {
      return false;
    }
  }

  /**
   * @brief It is a class that holds the logging properties.
   */
  class Properties final {
  public:
    inline static std::string FILE = "last_logs.log";
    inline static bool PRINT_TO_STDOUT = false, DISABLE = false;

    /**
     * @brief Change log file.
     * @param file New log file.
     * @param remove Will the old log file be deleted?
     */
    static void setFile(const std::string &file, bool remove = false) {
      moveOldLogs(FILE, file, remove);
      FILE = file;
    }

    /**
     * @brief Change whether logs are written to @c stdout.
     * @param state State (true or false).
     */
    static void setPrinting(bool state) { PRINT_TO_STDOUT = state; }

    /**
     * @brief Enable/disable logging.
     * @param state State (true or false).
     */
    static void setLogging(bool state) { DISABLE = !state; }

    /// @brief Reset logging properties to defaults.
    static void reset() {
      FILE = "last_logs.log";
      PRINT_TO_STDOUT = false;
      DISABLE = false;
    }
  };

  /// @brief The log text is finalized, and if necessary, write operations to a file or to @c stdout are performed.
  ~Logger() {
    if (Properties::DISABLE) return;

    const time_t t = time(nullptr);
    const tm *date = localtime(&t);
    std::ostringstream __oss;
    __oss << "<" << static_cast<char>(level) << "> [ "
          << "<on " << stripQuotes(std::filesystem::path(file).filename()) << ":" << line << "> " << date->tm_mday << "/"
          << date->tm_mon + 1 << "/" << date->tm_year + 1900 << " " << date->tm_hour << ":" << date->tm_min << ":" << date->tm_sec
          << "] " << function << "(): " << oss.str();
    std::string logLine = __oss.str();

    if (!std::filesystem::exists(logFile)) {
      if (std::ofstream tempFile(logFile, std::ios::out); !tempFile) {
        Properties::setFile("last_logs.log");
        LOG(INFO) << "Cannot create log file: " << logFile << ": " << strerror(errno) << std::endl;
        LOG(INFO) << "New logging file: last_logs.log" << std::endl;
      }
    }

    if (std::fstream fileStream(logFile, std::ios::app | std::ios::in | std::ios::out); fileStream) {
      if (std::filesystem::file_size(logFile) == 0)
        fileStream << std::string(46, '-') << std::endl
                   << " LOGGING BEGIN! LOGGING BEGIN! LOGGING BEGIN!" << std::endl
                   << std::string(46, '-') << std::endl;
      fileStream << logLine;
    } else {
      Properties::setFile("last_logs.log");
      LOG(INFO) << "Cannot write logs to log file: " << logFile << ": " << strerror(errno)
                << " Logging file setting up as: last_logs.log." << std::endl;
    }

    if (Properties::PRINT_TO_STDOUT) fprintf(stdout, "%s", logLine.c_str());
  }

  Logger() = delete;
  Logger(const Logger &) = delete;
  Logger(const LogLevels level, std::string function, std::string logFile, std::string file, int line)
      : level(level), function(std::move(function)), logFile(std::move(logFile)), file(std::move(file)), line(line) {}

  /// @brief To use the @c << operator for receiving non-function-like inputs.
  template <typename T> Logger &operator<<(const T &msg) {
    oss << msg;
    return *this;
  }

  /// @brief To use the @c << operator for receiving function-like inputs.
  Logger &operator<<(std::ostream &(*msg)(std::ostream &)) {
    oss << msg;
    return *this;
  }

  Logger &operator=(const Logger &) = delete;

  /// @brief Function for modern @c std::print style logging.
  template <typename... Args> Logger &write(std::format_string<Args...> fmt, Args &&...args) {
    oss << std::format(fmt, std::forward<Args>(args)...);
    return *this;
  }
};

} // namespace Helper

/**
 * @name Log level shorcuts.
 * @brief Shorcuts of logging levels.
 * @{
 */
inline Helper::LogLevels INFO = Helper::LogLevels::INFO;
inline Helper::LogLevels WARNING = Helper::LogLevels::WARNING;
inline Helper::LogLevels ERROR = Helper::LogLevels::ERROR;
inline Helper::LogLevels ABORT = Helper::LogLevels::ABORT;
/** @} */

/**
 * @namespace Out
 * @brief Output namespace.
 */
namespace Log {

/// @brief Parses the function name from a full function decleration (from std::source_location::function_name()).
inline std::string parseFunctionName(const std::string &full) {
  auto end = full.find('(');
  if (end == std::string_view::npos) return full;
  auto begin = full.rfind(':', end);
  if (begin == std::string_view::npos)
    begin = full.rfind(' ', end);
  else
    ++begin;
  if (begin == std::string_view::npos) return full.substr(0, end);
  return full.substr(begin, end - begin);
}

/// @brief Custom format struct for logger.
struct log_fmt {
  std::string_view fmt;
  std::source_location loc;

  consteval log_fmt(const char *f, std::source_location l = std::source_location::current()) : fmt(f), loc(l) {}
};

/// @brief Prints a formatted string to stdout.
template <typename... Args> inline void print(const std::format_string<Args...> &fmt, Args &&...args) {
  const std::string message = std::format(fmt, std::forward<Args>(args)...);
  fprintf(stdout, "%s", message.c_str());
}

/// @brief Prints a formatted string to stdout and appends a newline.
template <typename... Args> inline void println(const std::format_string<Args...> &fmt, Args &&...args) {
  const std::string message = std::format(fmt, std::forward<Args>(args)...);
  fprintf(stdout, "%s\n", message.c_str());
}

/// @brief Logs an @c INFO level message.
template <typename... Args> inline void info(const log_fmt &fmt, Args &&...args) {
  std::string message;
  try {
    message = std::vformat(fmt.fmt, std::make_format_args(args...));
  } catch (std::format_error &err) {
    Log::println("Failed to format string: {}", err.what());
    Log::println("This string format problem occurred on {}:{}():L{}", fmt.loc.file_name(), parseFunctionName(fmt.loc.function_name()),
                 fmt.loc.line());
    exit(EINVAL);
  }
  Helper::Logger(INFO, parseFunctionName(fmt.loc.function_name()), Helper::Logger::Properties::FILE.c_str(), fmt.loc.file_name(),
                 fmt.loc.line())
      << message << std::endl;
}

/// @brief Logs a @c WARNING level message.
template <typename... Args> inline void warning(const log_fmt &fmt, Args &&...args) {
  std::string message;
  try {
    message = std::vformat(fmt.fmt, std::make_format_args(args...));
  } catch (std::format_error &err) {
    Log::println("Failed to format string: {}", err.what());
    Log::println("This string format problem occurred on {}:{}():L{}", fmt.loc.file_name(), parseFunctionName(fmt.loc.function_name()),
                 fmt.loc.line());
    exit(EINVAL);
  }
  Helper::Logger(WARNING, parseFunctionName(fmt.loc.function_name()), Helper::Logger::Properties::FILE.c_str(), fmt.loc.file_name(),
                 fmt.loc.line())
      << message << std::endl;
}

/// @brief Logs an @c ERROR level message.
template <typename... Args> inline void error(const log_fmt &fmt, Args &&...args) {
  std::string message;
  try {
    message = std::vformat(fmt.fmt, std::make_format_args(args...));
  } catch (std::format_error &err) {
    Log::println("Failed to format string: {}", err.what());
    Log::println("This string format problem occurred on {}:{}():L{}", fmt.loc.file_name(), parseFunctionName(fmt.loc.function_name()),
                 fmt.loc.line());
    exit(EINVAL);
  }
  Helper::Logger(ERROR, parseFunctionName(fmt.loc.function_name()), Helper::Logger::Properties::FILE.c_str(), fmt.loc.file_name(),
                 fmt.loc.line())
      << message << std::endl;
}

/// @brief Logs an @c ABORT level message.
template <typename... Args> inline void abort(const log_fmt &fmt, Args &&...args) {
  std::string message;
  try {
    message = std::vformat(fmt.fmt, std::make_format_args(args...));
  } catch (std::format_error &err) {
    Log::println("Failed to format string: {}", err.what());
    Log::println("This string format problem occurred on {}:{}():L{}", fmt.loc.file_name(), parseFunctionName(fmt.loc.function_name()),
                 fmt.loc.line());
    exit(EINVAL);
  }
  Helper::Logger(INFO, parseFunctionName(fmt.loc.function_name()), Helper::Logger::Properties::FILE.c_str(), fmt.loc.file_name(),
                 fmt.loc.line())
      << message << std::endl;
}

} // namespace Log

#endif // #ifndef LIBHELPER_LOGGING_HPP
