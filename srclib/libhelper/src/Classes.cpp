/*
   Copyright 2026 Yağız Zengin

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

#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <libhelper/lib.hpp>
#include <sstream>

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

const char *Error::what() const noexcept { return _message.data(); }

Logger::Logger(const LogLevels level, const char *func, const char *file,
               const char *name, const char *source_file, const int line)
    : _level(level), _function_name(func), _logFile(file), _program_name(name),
      _file(source_file), _line(line) {}

Logger::~Logger() {
  if (LoggingProperties::DISABLE)
    return;

  std::ostringstream oss;
  oss << "<" << static_cast<char>(_level) << "> [ "
      << "<prog " << _program_name << "> "
      << "<on " << pathBasename(_file) << ":" << _line << "> " << currentDate() << " "
      << currentTime() << "] " << _function_name << "(): " << _oss.str() << std::endl;
  std::string logLine = oss.str();

  if (!isExists(_logFile)) {
    if (std::ofstream tempFile(_logFile, std::ios::out); !tempFile) {
#ifdef ANDROID_BUILD
      LoggingProperties::setLogFile("/tmp/last_pmt_logs.log")
#else
      LoggingProperties::setLogFile("last_logs.log");
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

  if (std::ofstream fileStream(_logFile, std::ios::app); fileStream) {
    fileStream << logLine;
  } else {
    LoggingProperties::setLogFile("last_logs.log");
    LOGN(HELPER, INFO) << "Cannot write logs to log file: " << _logFile << ": "
                       << strerror(errno)
                       << " Logging file setting up as: last_logs.log (this file)."
                       << std::endl;
  }

  if (LoggingProperties::PRINT)
    std::cout << logLine;
}

Logger &Logger::operator<<(std::ostream &(*msg)(std::ostream &)) {
  _oss << msg;
  return *this;
}

garbageCollector::~garbageCollector() {
  for (auto &ptr_func : _cleaners)
    ptr_func();
  for (const auto &fd : _fds)
    close(fd);
  for (const auto &fp : _fps)
    fclose(fp);
  for (const auto &dp : _dps)
    closedir(dp);
  for (const auto &file : _files)
    eraseEntry(file);
}

void garbageCollector::delFileAfterProgress(const std::string &_path) {
  _files.push_back(_path);
}
void garbageCollector::closeAfterProgress(const int _fd) { _fds.push_back(_fd); }
void garbageCollector::closeAfterProgress(FILE *_fp) { _fps.push_back(_fp); }
void garbageCollector::closeAfterProgress(DIR *_dp) { _dps.push_back(_dp); }

SilenceStdout::SilenceStdout() { silenceAgain(); }

SilenceStdout::~SilenceStdout() {
  if (saved_stdout != -1 && dev_null != -1)
    stop();
}

void SilenceStdout::stop() {
  fflush(stdout);
  dup2(saved_stdout, STDOUT_FILENO);
  close(saved_stdout);
  close(dev_null);
  saved_stdout = -1;
  dev_null = -1;
}

void SilenceStdout::silenceAgain() {
  fflush(stdout);
  saved_stdout = dup(STDOUT_FILENO);
  dev_null = open("/dev/null", O_WRONLY);
  dup2(dev_null, STDOUT_FILENO);
}

} // namespace Helper
