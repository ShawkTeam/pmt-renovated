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

#ifndef LIBHELPER_LIB_HPP
#define LIBHELPER_LIB_HPP

#include <cstdint>
#include <exception>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <dirent.h>

#ifndef ONLY_HELPER_MACROS

enum LogLevels {
  INFO = static_cast<int>('I'),
  WARNING = static_cast<int>('W'),
  ERROR = static_cast<int>('E'),
  ABORT = static_cast<int>('A')
};

constexpr mode_t DEFAULT_FILE_PERMS = 0644;
constexpr mode_t DEFAULT_EXTENDED_FILE_PERMS = 0755;
constexpr mode_t DEFAULT_DIR_PERMS = 0755;
constexpr int YES = 1;
constexpr int NO = 0;

namespace Helper {
// Logging
class Logger final {
private:
  LogLevels _level;
  std::ostringstream _oss;
  const char *_funcname, *_logFile, *_program_name, *_file;
  int _line;

public:
  Logger(LogLevels level, const char *func, const char *file, const char *name,
         const char *sfile, int line);

  ~Logger();

  template <typename T> Logger &operator<<(const T &msg) {
    _oss << msg;
    return *this;
  }

  Logger &operator<<(std::ostream &(*msg)(std::ostream &));
};

// Throwable error class
class Error final : public std::exception {
private:
  std::string _message;

public:
  __attribute__((format(printf, 2, 3))) explicit Error(const char *format, ...);

  [[nodiscard]] const char *what() const noexcept override;
};

// Close file descriptors and delete allocated array memory
class garbageCollector {
private:
  std::vector<char *> _ptrs_c;
  std::vector<uint8_t *> _ptrs_u;
  std::vector<FILE *> _fps;
  std::vector<DIR *> _dps;
  std::vector<int> _fds;
  std::vector<std::string> _files;

public:
  ~garbageCollector();

  void delAfterProgress(char *&_ptr);
  void delAfterProgress(uint8_t *&_ptr);
  void delFileAfterProgress(const std::string &path);
  void closeAfterProgress(FILE *&_fp);
  void closeAfterProgress(DIR *&_dp);
  void closeAfterProgress(int _fd);
};

namespace LoggingProperties {
extern std::string_view FILE, NAME;
extern bool PRINT, DISABLE;

void set(std::string_view name, std::string_view file);
void setProgramName(std::string_view name);
void setLogFile(std::string_view file);
void setPrinting(int state);
void setLoggingState(int state); // Disable/enable logging

void reset();
} // namespace LoggingProperties

// Checkers - don't throw Helper::Error
bool hasSuperUser();
bool hasAdbPermissions();
bool isExists(std::string_view entry);
bool fileIsExists(std::string_view file);
bool directoryIsExists(std::string_view directory);
bool linkIsExists(std::string_view entry);
bool isLink(std::string_view entry);
bool isSymbolicLink(std::string_view entry);
bool isHardLink(std::string_view entry);
bool areLinked(std::string_view entry1, std::string_view entry2);

// File I/O
bool writeFile(std::string_view file, std::string_view text);
std::optional<std::string> readFile(std::string_view file);

// Creators
bool makeDirectory(std::string_view path);
bool makeRecursiveDirectory(std::string_view paths);
bool createFile(std::string_view path);
bool createSymlink(std::string_view entry1, std::string_view entry2);

// Removers
bool eraseEntry(std::string_view entry);
bool eraseDirectoryRecursive(std::string_view directory);

// Getters
int64_t fileSize(std::string_view file);
std::string readSymlink(std::string_view entry);

// SHA-256
bool sha256Compare(std::string_view file1, std::string_view file2);
std::optional<std::string> sha256Of(std::string_view path);

// Utilities
bool copyFile(std::string_view file, std::string_view dest);
bool runCommand(std::string_view cmd);
bool confirmPropt(std::string_view message);
bool changeMode(std::string_view file, mode_t mode);
bool changeOwner(std::string_view file, uid_t uid, gid_t gid);
std::string currentWorkingDirectory();
std::string currentDate();
std::string currentTime();
std::string runCommandWithOutput(std::string_view cmd);
std::string pathJoin(std::string base, std::string relative);
std::string pathBasename(std::string_view entry);
std::string pathDirname(std::string_view entry);
uint64_t getRandomOffset(uint64_t size, uint64_t bufferSize);

#ifdef __ANDROID__
// Android
std::string getProperty(std::string_view prop);
bool reboot(std::string_view arg);
#endif

// Library-specif
std::string getLibVersion();

// Open input path with flags and add to integrity list. And return file
// descriptor
[[nodiscard]] int openAndAddToCloseList(const std::string_view &path,
                                        garbageCollector &collector, int flags,
                                        mode_t mode = 0000);
[[nodiscard]] FILE *openAndAddToCloseList(const std::string_view &path,
                                          garbageCollector &collector,
                                          const char *mode);
[[nodiscard]] DIR *openAndAddToCloseList(const std::string_view &path,
                                          garbageCollector &collector);

} // namespace Helper

#endif // #ifndef ONLY_HELPER_MACROS

#define HELPER "libhelper"

#define KB(x) (x * 1024)     // KB(8) = 8192 (8 * 1024)
#define MB(x) (KB(x) * 1024) // MB(4) = 4194304 (KB(4) * 1024)
#define GB(x) (MB(x) * 1024) // GB(1) = 1073741824 (MB(1) * 1024)

#define TO_KB(x) (x / 1024)        // TO_KB(1024) = 1
#define TO_MB(x) (TO_KB(x) / 1024) // TO_MB(2048) (2048 / 1024)
#define TO_GB(x) (TO_MB(x) / 1024) // TO_GB(1048576) (TO_MB(1048576) / 1024)

#define STYLE_RESET "\033[0m"
#define BOLD "\033[1m"
#define FAINT "\033[2m"
#define ITALIC "\033[3m"
#define UNDERLINE "\033[4m"
#define BLINC "\033[5m"
#define FAST_BLINC "\033[6m"
#define STRIKE_THROUGHT "\033[9m"
#define NO_UNDERLINE "\033[24m"
#define NO_BLINC "\033[25m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"

#ifndef NO_C_TYPE_HANDLERS
// ABORT(message), ex: ABORT("memory error!\n")
#define ABORT(msg)                                                             \
  do {                                                                         \
    fprintf(stderr, "%s%sCRITICAL ERROR%s: %s\nAborting...\n", BOLD, RED,      \
            STYLE_RESET, msg);                                                 \
    abort();                                                                   \
  } while (0)

// ERROR(message, exit), ex: ERROR("an error occured.\n", 1)
#define ERROR(msg, code)                                                       \
  do {                                                                         \
    fprintf(stderr, "%s%sERROR%s: %s", BOLD, RED, STYLE_RESET, msg);           \
    exit(code);                                                                \
  } while (0)

// WARNING(message), ex: WARNING("using default setting.\n")
#define WARNING(msg)                                                           \
  fprintf(stderr, "%s%sWARNING%s: %s", BOLD, YELLOW, STYLE_RESET, msg);

// INFO(message), ex: INFO("operation ended.\n")
#define INFO(msg)                                                              \
  fprintf(stdout, "%s%sINFO%s: %s", BOLD, GREEN, STYLE_RESET, msg);
#endif // #ifndef NO_C_TYPE_HANDLERS

#define LOG(level)                                                             \
  Helper::Logger(level, __func__, Helper::LoggingProperties::FILE.data(),      \
                 Helper::LoggingProperties::NAME.data(), __FILE__, __LINE__)
#define LOGN(name, level)                                                      \
  Helper::Logger(level, __func__, Helper::LoggingProperties::FILE.data(),      \
                 name, __FILE__, __LINE__)
#define LOGNF(name, file, level)                                               \
  Helper::Logger(level, file, name, __FILE__, __LINE__)

#define LOG_IF(level, condition)                                               \
  if (condition)                                                               \
  Helper::Logger(level, __func__, Helper::LoggingProperties::FILE.data(),      \
                 Helper::LoggingProperties::NAME.data(), __FILE__, __LINE__)
#define LOGN_IF(name, level, condition)                                        \
  if (condition)                                                               \
  Helper::Logger(level, __func__, Helper::LoggingProperties::FILE.data(),      \
                 name, __FILE__, __LINE__)
#define LOGNF_IF(name, file, level, condition)                                 \
  if (condition) Helper::Logger(level, __func__, file, name, __FILE__, __LINE__)

#ifdef ANDROID_BUILD
#define MKVERSION(name)                                                        \
  char vinfo[512];                                                             \
  sprintf(vinfo,                                                               \
          "%s 1.2.0\nBuildType: Release\nCompiler: clang\n"                    \
          "BuildFlags: -Wall;-Werror;-Wno-deprecated-declarations;-Os",        \
          name);                                                               \
  return std::string(vinfo)
#else
#define MKVERSION(name)                                                        \
  char vinfo[512];                                                             \
  sprintf(vinfo,                                                               \
          "%s %s [%s %s]\nBuildType: %s\nCMakeVersion: %s\nCompilerVersion: "  \
          "%s\nBuildFlags: %s",                                                \
          name, BUILD_VERSION, BUILD_DATE, BUILD_TIME, BUILD_TYPE,             \
          BUILD_CMAKE_VERSION, BUILD_COMPILER_VERSION, BUILD_FLAGS);           \
  return std::string(vinfo)
#endif

#endif // #ifndef LIBHELPER_LIB_HPP
