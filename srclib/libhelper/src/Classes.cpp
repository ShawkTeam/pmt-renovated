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

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <libgen.h>
#include <libhelper/lib.hpp>
#include <sstream>
#include <unistd.h>

namespace Helper {
Error::Error(const char *format, ...) {
  char buf[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  _message = std::string(buf);
  LOGN(HELPER, ERROR) << _message << std::endl;
}

const char *Error::what() const noexcept { return _message.data(); }

Logger::Logger(const LogLevels level, const char *func, const char *file,
               const char *name, const char *sfile, const int line)
    : _level(level), _funcname(func), _logFile(file), _program_name(name),
      _file(sfile), _line(line) {}

Logger::~Logger() {
  if (LoggingProperties::DISABLE) return;
  char str[1024];
  snprintf(str, sizeof(str), "<%c> [ <prog %s> <on %s:%d> %s %s] %s(): %s",
           static_cast<char>(_level), _program_name,
           basename(const_cast<char *>(_file)), _line, currentDate().data(),
           currentTime().data(), _funcname, _oss.str().data());

  if (!isExists(_logFile)) {
    if (const int fd =
            open(_logFile, O_WRONLY | O_CREAT, DEFAULT_EXTENDED_FILE_PERMS);
        fd != -1)
      close(fd);
    else {
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

  if (FILE *fp = fopen(_logFile, "a"); fp != nullptr) {
    fprintf(fp, "%s", str);
    fclose(fp);
  } else {
    LoggingProperties::setLogFile("last_logs.log");
    LOGN(HELPER, INFO)
        << "Cannot write logs to log file: " << _logFile << ": "
        << strerror(errno)
        << " Logging file setting up as: last_logs.log (this file)."
        << std::endl;
  }

  if (LoggingProperties::PRINT) printf("%s", str);
}

Logger &Logger::operator<<(std::ostream &(*msg)(std::ostream &)) {
  _oss << msg;
  return *this;
}

garbageCollector::~garbageCollector() {
  for (const auto &ptr : _ptrs_c)
    delete[] ptr;
  for (const auto &ptr : _ptrs_u)
    delete[] ptr;
  for (const auto &fd : _fds)
    close(fd);
  for (const auto &fp : _fps)
    fclose(fp);
  for (const auto &file : _files)
    eraseEntry(file);
}

void garbageCollector::delAfterProgress(char *&_ptr) {
  _ptrs_c.push_back(_ptr);
}
void garbageCollector::delAfterProgress(uint8_t *&_ptr) {
  _ptrs_u.push_back(_ptr);
}
void garbageCollector::delFileAfterProgress(const std::string &path) {
  _files.push_back(path);
}
void garbageCollector::closeAfterProgress(const int _fd) {
  _fds.push_back(_fd);
}
void garbageCollector::closeAfterProgress(FILE *&_fp) { _fps.push_back(_fp); }
} // namespace Helper
