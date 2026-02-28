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

#ifndef LIBHELPER_LOGGING_HPP
#define LIBHELPER_LOGGING_HPP

#define LOG(level)                                                                                                                    \
  Helper::Logger(level, __func__, Helper::Logger::Properties::FILE.c_str(), Helper::Logger::Properties::NAME.c_str(), __FILE__,       \
                 __LINE__)
#define LOGF(file, level) Helper::Logger(level, __func__, file, Helper::Logger::Properties::NAME.c_str(), __FILE__, __LINE__)
#define LOGN(name, level) Helper::Logger(level, __func__, Helper::Logger::Properties::FILE.c_str(), name, __FILE__, __LINE__)
#define LOGNF(name, file, level) Helper::Logger(level, __func__, file, name, __FILE__, __LINE__)

#define LOG_IF(level, condition)                                                                                                      \
  if (!(condition)) {                                                                                                                 \
  } else                                                                                                                              \
    Helper::Logger(level, __func__, Helper::Logger::Properties::FILE.c_str(), Helper::Logger::Properties::NAME.c_str(), __FILE__,     \
                   __LINE__)
#define LOGF_IF(file, level, condition)                                                                                               \
  if (!(condition)) {                                                                                                                 \
  } else                                                                                                                              \
    Helper::Logger(level, __func__, file, Helper::Logger::Properties::NAME.c_str(), __FILE__, __LINE__)
#define LOGN_IF(name, level, condition)                                                                                               \
  if (!(condition)) {                                                                                                                 \
  } else                                                                                                                              \
    Helper::Logger(level, __func__, Helper::Logger::Properties::FILE.c_str(), name, __FILE__, __LINE__)
#define LOGNF_IF(name, file, level, condition)                                                                                        \
  if (!(condition)) {                                                                                                                 \
  } else                                                                                                                              \
    Helper::Logger(level, __func__, file, name, __FILE__, __LINE__)

#include <sstream>
#include <string>
#include <fstream>
#include <libhelper/macros.hpp>

namespace Helper {

enum LogLevels {
  INFO = static_cast<int>('I'),
  WARNING = static_cast<int>('W'),
  ERROR = static_cast<int>('E'),
  ABORT = static_cast<int>('A')
};

// Logger class for logging
class Logger {
  LogLevels level;
  std::ostringstream oss;
  std::string function, logFile, name, file;
  int line;

public:
  class Properties final {
  public:
    inline static std::string FILE = "last_logs.log", NAME = "main";
    inline static bool PRINT_TO_STDOUT = false, DISABLE = false;

    static void setFile(const std::string &file) { FILE = file; }
    static void setName(const std::string &name) { NAME = name; }
    static void setPrinting(bool state) { PRINT_TO_STDOUT = state; }
    static void setLogging(bool state) { DISABLE = !state; }

    static void reset() {
      FILE = "last_logs.log";
      NAME = "main";
      PRINT_TO_STDOUT = false;
      DISABLE = false;
    }
  };

  ~Logger() {
    if (Properties::DISABLE) return;

    const time_t t = time(nullptr);
    const tm *date = localtime(&t);
    std::ostringstream __oss;
    __oss << "<" << static_cast<char>(level) << "> [ "
          << "<prog " << name << "> "
          << "<on " << std::filesystem::path(file).filename() << ":" << line << "> " << date->tm_mday << "/" << date->tm_mon + 1 << "/"
          << date->tm_year + 1900 << " " << date->tm_hour << ":" << date->tm_min << ":" << date->tm_sec << "] " << function
          << "(): " << oss.str();
    std::string logLine = __oss.str();

    if (!std::filesystem::exists(logFile)) {
      if (std::ofstream tempFile(logFile, std::ios::out); !tempFile) {
        Properties::setFile("last_logs.log");
        LOGN(HELPER, INFO) << "Cannot create log file: " << logFile << ": " << strerror(errno) << std::endl;
        LOGN(HELPER, INFO) << "New logging file: last_logs.log" << std::endl;
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
      LOGN(HELPER, INFO) << "Cannot write logs to log file: " << logFile << ": " << strerror(errno)
                         << " Logging file setting up as: last_logs.log." << std::endl;
    }

    if (Properties::PRINT_TO_STDOUT) fprintf(stdout, "%s", logLine.c_str());
  }

  Logger() = delete;
  Logger(const LogLevels level, std::string function, std::string logFile, std::string name, std::string file, int line)
      : level(level), function(std::move(function)), logFile(std::move(logFile)), name(std::move(name)), file(std::move(file)),
        line(line) {}

  template <typename T> Logger &operator<<(const T &msg) {
    oss << msg;
    return *this;
  }

  Logger &operator<<(std::ostream &(*msg)(std::ostream &)) {
    oss << msg;
    return *this;
  }
};

} // namespace Helper

inline Helper::LogLevels INFO = Helper::LogLevels::INFO;
inline Helper::LogLevels WARNING = Helper::LogLevels::WARNING;
inline Helper::LogLevels ERROR = Helper::LogLevels::ERROR;
inline Helper::LogLevels ABORT = Helper::LogLevels::ABORT;

#endif // #ifndef LIBHELPER_LOGGING_HPP
