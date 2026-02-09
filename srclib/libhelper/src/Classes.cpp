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

#include <cerrno>
#include <cstdarg>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <sstream>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <libhelper/lib.hpp>

namespace Helper {
Error::Error(const char *format, ...) {
  va_list args;

  va_start(args, format);
  int size = vsnprintf(nullptr, 0, format, args);
  va_end(args);

  if (size > 0) {
    std::vector<char> buf(size + 1);
    va_start(args, format);
    vsnprintf(buf.data(), buf.size(), format, args);
    va_end(args);

    _message = std::string(buf.data());
  }

  LOGN(HELPER, ERROR) << _message << std::endl;
}

Error &Error::operator<<(std::ostream &(*msg)(std::ostream &)) {
  _oss << msg;
  _message = _oss.str();
  return *this;
}

const char *Error::what() const noexcept { return _message.data(); }

Logger::Logger(const LogLevels level, const char *func, const char *file, const char *name, const char *source_file, const int line)
    : _level(level), _function_name(func), _logFile(file), _program_name(name), _file(source_file), _line(line) {}

Logger::~Logger() {
  if (LoggingProperties::DISABLE) return;

  std::ostringstream oss;
  oss << "<" << static_cast<char>(_level) << "> [ "
      << "<prog " << _program_name << "> "
      << "<on " << pathBasename(_file) << ":" << _line << "> " << currentDate() << " " << currentTime() << "] " << _function_name
      << "(): " << _oss.str();
  std::string logLine = oss.str();

  if (!isExists(_logFile)) {
    if (std::ofstream tempFile(_logFile, std::ios::out); !tempFile) {
#ifdef ANDROID_BUILD
      LoggingProperties::setLogFile("/tmp/last_pmt_logs.log")
#else
      LoggingProperties::setLogFile("/sdcard/last_logs.log");
#endif
              LOGN(HELPER, INFO)
          << "Cannot create log file: " << _logFile << ": " << strerror(errno)
#ifdef ANDROID_BUILD
          << " New logging file: /tmp/last_pmt_logs.log (this file)."
#else
          << " New logging file: last_logs.log (this file)."
#endif
          << std::endl;
    }
  }

  if (std::fstream fileStream(_logFile, std::ios::app | std::ios::in | std::ios::out); fileStream) {
    if (std::filesystem::file_size(_logFile) == 0)
      fileStream << std::string(46, '-') << std::endl
                 << " LOGGING BEGIN! LOGGING BEGIN! LOGGING BEGIN!" << std::endl
                 << std::string(46, '-') << std::endl;
    fileStream << logLine;
  } else {
    LoggingProperties::setLogFile("last_logs.log");
    LOGN(HELPER, INFO) << "Cannot write logs to log file: " << _logFile << ": " << strerror(errno)
                       << " Logging file setting up as: last_logs.log (this file)." << std::endl;
  }

  if (LoggingProperties::PRINT) std::cout << logLine;
}

Logger &Logger::operator<<(std::ostream &(*msg)(std::ostream &)) {
  _oss << msg;
  return *this;
}

garbageCollector::~garbageCollector() {
  for (auto &__cleaner : std::ranges::reverse_view(__cleaners))
    __cleaner();
}

void garbageCollector::delFileAfterProgress(const std::filesystem::path &__path) {
  __cleaners.emplace_back([__path] { std::filesystem::remove(__path); });
}
void garbageCollector::closeAfterProgress(int __fd) {
  __cleaners.emplace_back([__fd] {
    if (__fd >= 0) close(__fd);
  });
}
void garbageCollector::closeAfterProgress(FILE *__fp) {
  __cleaners.emplace_back([__fp] { CleanupTraits<FILE>::cleanup(__fp); });
}
void garbageCollector::closeAfterProgress(DIR *__dp) {
  __cleaners.emplace_back([__dp] { CleanupTraits<DIR>::cleanup(__dp); });
}

Silencer::Silencer() { silenceAgain(); }

Silencer::~Silencer() {
  if (saved_stdout != -1 && dev_null != -1) stop();
}

void Silencer::stop() {
  if (saved_stdout == -1 && saved_stderr == -1 && dev_null == -1) return;
  fflush(stdout);
  fflush(stderr);
  dup2(saved_stdout, STDOUT_FILENO);
  dup2(saved_stderr, STDERR_FILENO);
  close(saved_stdout);
  close(saved_stderr);
  close(dev_null);
  saved_stdout = -1;
  saved_stderr = -1;
  dev_null = -1;
}

void Silencer::silenceAgain() {
  if (saved_stdout != -1 && saved_stderr != -1 && dev_null != -1) return;
  fflush(stdout);
  fflush(stderr);
  saved_stdout = dup(STDOUT_FILENO);
  saved_stderr = dup(STDERR_FILENO);
  dev_null = open("/dev/null", O_WRONLY);
  dup2(dev_null, STDOUT_FILENO);
  dup2(dev_null, STDERR_FILENO);
}

} // namespace Helper
