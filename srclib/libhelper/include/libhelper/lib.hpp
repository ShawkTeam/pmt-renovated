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
#include <dirent.h>
#include <exception>
#include <functional>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#define KB(x) (static_cast<uint64_t>(x) * 1024) // KB(8) = 8192 (8 * 1024)
#define MB(x) (KB(x) * 1024) // MB(4) = 4194304 (KB(4) * 1024)
#define GB(x) (MB(x) * 1024) // GB(1) = 1073741824 (MB(1) * 1024)

#define TO_KB(x) (x / 1024)        // TO_KB(1024) = 1
#define TO_MB(x) (TO_KB(x) / 1024) // TO_MB(2048) (2048 / 1024)
#define TO_GB(x) (TO_MB(x) / 1024) // TO_GB(1048576) (TO_MB(1048576) / 1024)

#ifndef ONLY_HELPER_MACROS

enum LogLevels {
  INFO = static_cast<int>('I'),
  WARNING = static_cast<int>('W'),
  ERROR = static_cast<int>('E'),
  ABORT = static_cast<int>('A')
};

enum sizeCastTypes {
  B = static_cast<int>('B'),
  KB = static_cast<int>('K'),
  MB = static_cast<int>('M'),
  GB = static_cast<int>('G')
};

constexpr mode_t DEFAULT_FILE_PERMS = 0644;
constexpr mode_t DEFAULT_EXTENDED_FILE_PERMS = 0755;
constexpr mode_t DEFAULT_DIR_PERMS = 0755;
constexpr int YES = 1;
constexpr int NO = 0;

namespace Helper {
// Throwable error class
class Error final : public std::exception {
private:
  std::string _message;
  std::ostringstream _oss;

public:
  __attribute__((format(printf, 2, 3))) explicit Error(const char *format, ...);

  [[nodiscard]] const char *what() const noexcept override;
};

// Logging
class Logger final {
private:
  LogLevels _level;
  std::ostringstream _oss;
  const char *_function_name, *_logFile, *_program_name, *_file;
  int _line;

public:
  Logger(LogLevels level, const char *func, const char *file, const char *name,
         const char *source_file, int line);

  ~Logger();

  template <typename T> Logger &operator<<(const T &msg) {
    _oss << msg;
    return *this;
  }

  Logger &operator<<(std::ostream &(*msg)(std::ostream &));
};

// Close file descriptors and delete allocated array memory
class garbageCollector {
private:
  std::vector<std::function<void()>> _cleaners;
  std::vector<FILE *> _fps;
  std::vector<DIR *> _dps;
  std::vector<int> _fds;
  std::vector<std::string> _files;

public:
  ~garbageCollector();

  template <typename T> void delAfterProgress(T *_ptr) {
    _cleaners.push_back([_ptr] { delete[] _ptr; });
  }

  void delFileAfterProgress(const std::string &_path);
  void closeAfterProgress(FILE *_fp);
  void closeAfterProgress(DIR *_dp);
  void closeAfterProgress(int _fd);
};

template <int max = 100, int start = 0, int count = 10, int d = 0>
class Random {
  static_assert(max > start, "max is larger than start");
  static_assert(count > 1, "count is larger than 1");
  static_assert(count <= max - start, "count is greater than max-start");

public:
  static std::set<int> get() {
    std::set<int> set;
    std::random_device rd;
    std::mt19937 gen(rd());

    if constexpr (d > 0) {
      std::uniform_int_distribution<> dist(0, (max - start - 1) / d);
      while (set.size() < count)
        set.insert(start + dist(gen) * d);
    } else {
      std::uniform_int_distribution<> dist(start, max - 1);
      while (set.size() < count)
        set.insert(dist(gen));
    }

    return set;
  }

  static int getNumber() {
    std::random_device rd;
    std::mt19937 gen(rd());
    int ret;

    if constexpr (d > 0) {
      std::uniform_int_distribution<> dist(0, (max - start - 1) / d);
      ret = start + dist(gen) * d;
    } else {
      std::uniform_int_distribution<> dist(start, max - 1); // max exclusive
      ret = dist(gen);
    }

    return ret;
  }
};

template <typename _Type1, typename _Type2, typename _Type3> class PureTuple {
private:
  void expand_if_needed() {
    if (count == capacity) {
      capacity *= 2;
      Data *data = new Data[capacity];

      for (size_t i = 0; i < count; i++)
        data[i] = tuple_data[i];

      delete[] tuple_data;
      tuple_data = data;
    }
  }

public:
  struct Data {
    _Type1 first;
    _Type2 second;
    _Type3 third;

    bool
    operator==(const std::tuple<_Type1, _Type2, _Type3> &t) const noexcept {
      return first == std::get<0>(t) && second == std::get<1>(t) &&
             third == std::get<2>(t);
    }

    bool operator==(const Data &other) const noexcept {
      return first == other.first && second == other.second &&
             third == other.third;
    }

    bool operator!=(const Data &other) const noexcept {
      return !(*this == other);
    }

    explicit operator bool() const noexcept {
      return first != _Type1{} || second != _Type2{} || third != _Type3{};
    }

    bool operator!() const noexcept { return !bool{*this}; }

    void operator()(const std::tuple<_Type1, _Type2, _Type3> &t) {
      first = std::get<0>(t);
      second = std::get<1>(t);
      third = std::get<2>(t);
    }

    Data &operator=(const std::tuple<_Type1, _Type2, _Type3> &t) {
      first = std::get<0>(t);
      second = std::get<1>(t);
      third = std::get<2>(t);
      return *this;
    }
  };

  Data *tuple_data = nullptr;
  Data tuple_data_type = {_Type1{}, _Type2{}, _Type3{}};
  size_t capacity{}, count{};

  PureTuple() : tuple_data(new Data[20]), capacity(20), count(0) {}
  ~PureTuple() { delete[] tuple_data; }

  PureTuple(std::initializer_list<Data> val)
      : tuple_data(new Data[20]), capacity(20), count(0) {
    for (const auto &v : val)
      insert(v);
  }
  PureTuple(PureTuple &other)
      : tuple_data(new Data[other.capacity]), capacity(other.capacity),
        count(other.count) {
    std::copy(other.tuple_data, other.tuple_data + count, tuple_data);
  }
  PureTuple(PureTuple &&other) noexcept
      : tuple_data(new Data[other.capacity]), capacity(other.capacity),
        count(other.count) {
    std::copy(other.tuple_data, other.tuple_data + count, tuple_data);
    other.clear();
  }

  class iterator {
  private:
    Data *it;

  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Data;
    using difference_type = std::ptrdiff_t;
    using pointer = Data *;
    using reference = Data &;

    explicit iterator(Data *ptr) : it(ptr) {}

    pointer operator->() const { return it; }
    reference operator*() { return *it; }

    iterator &operator++() {
      ++it;
      return *this;
    }
    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    iterator &operator--() {
      --it;
      return *this;
    }
    iterator operator--(int) {
      iterator tmp = *this;
      --(*this);
      return tmp;
    }

    iterator &operator+=(difference_type n) {
      it += n;
      return *this;
    }
    iterator operator+(difference_type n) const { return iterator(it + n); }
    iterator &operator-=(difference_type n) {
      it -= n;
      return *this;
    }
    iterator operator-(difference_type n) const { return iterator(it - n); }
    difference_type operator-(const iterator &other) const {
      return it - other.it;
    }

    reference operator[](difference_type n) const { return it[n]; }

    bool operator<(const iterator &other) const { return it < other.it; }
    bool operator>(const iterator &other) const { return it > other.it; }
    bool operator<=(const iterator &other) const { return it <= other.it; }
    bool operator>=(const iterator &other) const { return it >= other.it; }

    bool operator!=(const iterator &other) const { return it != other.it; }
    bool operator==(const iterator &other) const { return it == other.it; }
  };

  bool find(const Data &data) const noexcept {
    for (size_t i = 0; i < count; i++)
      if (data == tuple_data[i]) return true;

    return false;
  }

  template <typename T = std::tuple<_Type1, _Type2, _Type3>>
  std::enable_if_t<std::is_same_v<T, std::tuple<_Type1, _Type2, _Type3>>, bool>
  find(const std::tuple<_Type1, _Type2, _Type3> &t) const noexcept {
    for (size_t i = 0; i < count; i++)
      if (tuple_data[i] == t) return true;

    return false;
  }

  bool find(const _Type1 &val, const _Type2 &val2,
            const _Type3 &val3) const noexcept {
    for (size_t i = 0; i < count; i++)
      if (tuple_data[i] == std::make_tuple(val, val2, val3)) return true;

    return false;
  }

  void insert(const Data &val) noexcept {
    expand_if_needed();
    if (!find(val)) tuple_data[count++] = val;
  }

  template <typename T = std::tuple<_Type1, _Type2, _Type3>>
  std::enable_if_t<std::is_same_v<T, std::tuple<_Type1, _Type2, _Type3>>, void>
  insert(const std::tuple<_Type1, _Type2, _Type3> &t) noexcept {
    expand_if_needed();
    if (!find(t))
      tuple_data[count++] =
          Data{std::get<0>(t), std::get<1>(t), std::get<2>(t)};
  }

  void insert(const _Type1 &val, const _Type2 &val2,
              const _Type3 &val3) noexcept {
    expand_if_needed();
    if (!find(val, val2, val3)) tuple_data[count++] = Data{val, val2, val3};
  }

  void merge(const PureTuple &other) noexcept {
    for (const auto &v : other)
      insert(v);
  }

  void pop_back() noexcept {
    if (count > 0) --count;
  }

  void pop(const Data &data) noexcept {
    for (size_t i = 0; i < count; i++) {
      if (tuple_data[i] == data) {
        for (size_t j = i; j < count - 1; j++)
          tuple_data[j] = tuple_data[j + 1];
        --count;
        break;
      }
    }
  }

  void pop(const size_t i) noexcept {
    if (i >= count) return;
    for (size_t x = 0; x < count; x++) {
      if (i == x) {
        for (size_t j = i; j < count - 1; j++)
          tuple_data[j] = tuple_data[j + 1];
        --count;
        break;
      }
    }
  }

  void pop(const _Type1 &val, const _Type2 &val2, const _Type3 &val3) noexcept {
    for (size_t i = 0; i < count; i++) {
      if (tuple_data[i] == std::make_tuple(val, val2, val3)) {
        for (size_t j = i; j < count - 1; j++)
          tuple_data[j] = tuple_data[j + 1];
        --count;
        break;
      }
    }
  }

  template <typename T = std::tuple<_Type1, _Type2, _Type3>>
  std::enable_if_t<std::is_same_v<T, std::tuple<_Type1, _Type2, _Type3>>, void>
  pop(const std::tuple<_Type1, _Type2, _Type3> &t) noexcept {
    for (size_t i = 0; i < count; i++) {
      if (tuple_data[i] == t) {
        for (size_t j = i; j < count - 1; j++)
          tuple_data[j] = tuple_data[j + 1];
        --count;
        break;
      }
    }
  }

  void clear() noexcept {
    delete[] tuple_data;
    count = 0;
    capacity = 20;
    tuple_data = new Data[capacity];
  }

  Data back() const noexcept {
    return (count > 0) ? tuple_data[count - 1] : Data{};
  }
  Data top() const noexcept { return (count > 0) ? tuple_data[0] : Data{}; }

  Data at(size_t i) const noexcept {
    if (i >= count) return Data{};
    return tuple_data[i];
  }

  void foreach (std::function<void(_Type1, _Type2, _Type3)> func) {
    for (size_t i = 0; i < count; i++)
      func(tuple_data[i].first, tuple_data[i].second, tuple_data[i].third);
  }

  void foreach (std::function<void(std::tuple<_Type1, _Type2, _Type3>)> func) {
    for (size_t i = 0; i < count; i++)
      func(std::make_tuple(tuple_data[i].first, tuple_data[i].second,
                           tuple_data[i].third));
  }

  [[nodiscard]] size_t size() const noexcept { return count; }
  [[nodiscard]] bool empty() const noexcept { return count == 0; }

  iterator begin() const noexcept { return iterator(tuple_data); }
  iterator end() const noexcept { return iterator(tuple_data + count); }

  explicit operator bool() const noexcept { return count > 0; }
  bool operator!() const noexcept { return count == 0; }

  bool operator==(const PureTuple &other) const noexcept {
    if (this->count != other.count || this->capacity != other.capacity)
      return false;

    for (size_t i = 0; i < this->count; i++)
      if (tuple_data[i] != other.tuple_data[i]) return false;

    return true;
  }
  bool operator!=(const PureTuple &other) const noexcept {
    return !(*this == other);
  }

  Data operator[](size_t i) const noexcept {
    if (i >= count) return Data{};
    return tuple_data[i];
  }
  explicit operator int() const noexcept { return count; }

  PureTuple &operator=(const PureTuple &other) {
    if (this != &other) {
      delete[] tuple_data;

      capacity = other.capacity;
      count = other.count;
      tuple_data = new Data[capacity];

      std::copy(other.tuple_data, other.tuple_data + count, tuple_data);
    }

    return *this;
  }

  PureTuple &operator<<(const std::tuple<_Type1, _Type2, _Type3> &t) noexcept {
    insert(t);
    return *this;
  }

  friend PureTuple &operator>>(const std::tuple<_Type1, _Type2, _Type3> &t,
                               PureTuple &tuple) noexcept {
    tuple.insert(t);
    return tuple;
  }
};

namespace LoggingProperties {
extern std::string_view FILE, NAME;
extern bool PRINT, DISABLE;

void set(std::string_view name, std::string_view file);
void setProgramName(std::string_view name);
void setLogFile(std::string_view file);

template <int state> void setPrinting() {
  if (state == 1 || state == 0) PRINT = state;
  else PRINT = NO;
}
template <int state> void setLoggingState() {
  if (state == 1 || state == 0) DISABLE = state;
  else DISABLE = NO;
}

void reset();
} // namespace LoggingProperties

// -------------------------------
// Checkers - not throws Helper::Error
// -------------------------------

/**
 * It is checked whether the user ID used is equivalent to AID_ROOT.
 * See include/private/android_filesystem_config.h
 */
bool hasSuperUser();

/**
 * It is checked whether the user ID used is equivalent to AID_SHELL.
 * See include/private/android_filesystem_config.h
 */
bool hasAdbPermissions();

/**
 * Checks whether the file/directory exists.
 */
bool isExists(std::string_view entry);

/**
 * Checks whether the file exists.
 */
bool fileIsExists(std::string_view file);

/**
 * Checks whether the directory exists.
 */
bool directoryIsExists(std::string_view directory);

/**
 * Checks whether the link (symbolic or hard) exists.
 */
bool linkIsExists(std::string_view entry);

/**
 * Checks if the entry is a symbolic link.
 */
bool isLink(std::string_view entry);

/**
 * Checks if the entry is a symbolic link.
 */
bool isSymbolicLink(std::string_view entry);

/**
 * Checks if the entry is a hard link.
 */
bool isHardLink(std::string_view entry);

/**
 * Checks whether entry1 is linked to entry2.
 */
bool areLinked(std::string_view entry1, std::string_view entry2);

// -------------------------------
// File I/O - not throws Helper::Error
// -------------------------------

/**
 * Writes given text into file.
 * If file does not exist, it is automatically created.
 * Returns true on success.
 */
bool writeFile(std::string_view file, std::string_view text);

/**
 * Reads file content into string.
 * On success returns file content.
 * On error returns std::nullopt.
 */
std::optional<std::string> readFile(std::string_view file);

// -------------------------------
// Creators
// -------------------------------

/**
 * Create directory.
 */
bool makeDirectory(std::string_view path);

/**
 * Create recursive directory.
 */
bool makeRecursiveDirectory(std::string_view paths);

/**
 * Create file.
 */
bool createFile(std::string_view path);

/**
 * Symlink entry1 to entry2.
 */
bool createSymlink(std::string_view entry1, std::string_view entry2);

// -------------------------------
// Removers - not throws Helper::Error
// -------------------------------

/**
 * Remove file or empty directory.
 */
bool eraseEntry(std::string_view entry);

/**
 * Remove directory and all directory contents recursively.
 */
bool eraseDirectoryRecursive(std::string_view directory);

// -------------------------------
// Getters - not throws Helper::Error
// -------------------------------

/**
 * Get file size.
 */
int64_t fileSize(std::string_view file);

/**
 * Read symlinks.
 */
std::string readSymlink(std::string_view entry);

// -------------------------------
// SHA-256
// -------------------------------

/**
 * Compare SHA-256 values SHA-256 of files.
 * Throws Helper::Error on error occurred.
 */
bool sha256Compare(std::string_view file1, std::string_view file2);

/**
 * Get SHA-256 of file.
 * Throws Helper::Error on error occurred.
 */
std::optional<std::string> sha256Of(std::string_view path);

// -------------------------------
// Utilities - not throws Helper::Error
// -------------------------------

/**
 * Copy file to dest.
 */
bool copyFile(std::string_view file, std::string_view dest);

/**
 * Run shell command.
 */
bool runCommand(std::string_view cmd);

/**
 * Shows message and asks for y/N from user.
 */
bool confirmPropt(std::string_view message);

/**
 * Change file permissions.
 */
bool changeMode(std::string_view file, mode_t mode);

/**
 * Change file owner (user ID and group ID).
 */
bool changeOwner(std::string_view file, uid_t uid, gid_t gid);

/**
 * Get current working directory as string.
 * Returns empty string on error.
 */
std::string currentWorkingDirectory();

/**
 * Get current date as string (format: YYYY-MM-DD).
 * Returns empty string on error.
 */
std::string currentDate();

/**
 * Get current time as string (format: HH:MM:SS).
 * Returns empty string on error.
 */
std::string currentTime();

/**
 * Run shell command return output as string.
 * Returns std::pair<std::string, int>.
 */
std::pair<std::string, int> runCommandWithOutput(std::string_view cmd);

/**
 * Joins base path with relative path and returns result.
 */
std::string pathJoin(std::string base, std::string relative);

/**
 * Get the filename part of given path.
 */
std::string pathBasename(std::string_view entry);

/**
 * Get the directory part of given path.
 */
std::string pathDirname(std::string_view entry);

/**
 * Get random offset depending on size and bufferSize.
 */
uint64_t getRandomOffset(uint64_t size, uint64_t bufferSize);

/**
 * Convert input size to input multiple.
 */
int convertTo(uint64_t size, sizeCastTypes type);

/**
 * Convert input multiple variable to string.
 */
std::string multipleToString(sizeCastTypes type);

/**
 * Format it input and return as std::string.
 */
__attribute__((format(printf, 1, 2))) std::string format(const char *format,
                                                         ...);

/**
 * Convert input size to input multiple
 */
template <uint64_t size> int convertTo(const sizeCastTypes type) {
  if (type == KB) return TO_KB(size);
  if (type == MB) return TO_MB(size);
  if (type == GB) return TO_GB(size);
  return static_cast<int>(size);
}

// -------------------------------
// Android - not throws Helper::Error
// -------------------------------
#ifdef __ANDROID__
/**
 * Get input property as string (for Android).
 */
std::string getProperty(std::string_view prop);

/**
 * Reboot device to input mode (for Android).
 */
bool reboot(std::string_view arg);
#endif

/**
 * Get libhelper library version string.
 */
std::string getLibVersion();

/**
 * Open input path with flags and add to integrity list.
 * And returns file descriptor.
 */
[[nodiscard]] int openAndAddToCloseList(const std::string_view &path,
                                        garbageCollector &collector, int flags,
                                        mode_t mode = 0000);
/**
 * Open input path with flags and add to integrity list.
 * And returns file pointer.
 */
[[nodiscard]] FILE *openAndAddToCloseList(const std::string_view &path,
                                          garbageCollector &collector,
                                          const char *mode);
/**
 * Open input directory and add to integrity list.
 * And returns directory pointer.
 */
[[nodiscard]] DIR *openAndAddToCloseList(const std::string_view &path,
                                         garbageCollector &collector);

} // namespace Helper

#endif // #ifndef ONLY_HELPER_MACROS

#define HELPER "libhelper"

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
#define LOGF(file, level)                                                      \
  Helper::Logger(level, __func__, file,                                        \
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
#define LOGF_IF(file, level, condition)                                        \
  if (condition)                                                               \
  Helper::Logger(level, __func__, file,                                        \
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
          "%s 1.3.0\nCompiler: clang\n"                                        \
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
