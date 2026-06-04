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
 * @file management.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Smart classes, wrappers, and more.
 */

#ifndef LIBHELPER_MANAGEMENT_HPP
#define LIBHELPER_MANAGEMENT_HPP

#if __cplusplus < 202002L
#error "libhelper/management.hpp is requires C++20 or higher C++ standarts."
#endif

#include <type_traits>
#include <utility>
#include <vector>
#include <functional>
#include <future>
#include <sstream>
#include <iostream>
#include <ranges>
#include <filesystem>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libhelper/error.hpp>

namespace Helper {

/**
 * @brief It offers a ready-made @c cleanup() function. It is used by other classes.
 *        The default defination takes the type pointer and clears it with @c delete.
 *
 * @tparam T Type.
 * @note This class has overloads for @c DIR, @c FILE, and arrays. However, they are not included in this documentation. Please refer
 * to the code.
 */
template <typename T> struct CleanupTraits {
  static void cleanup(const T *ptr) { delete ptr; }
};

template <typename T> struct CleanupTraits<T[]> {
  static void cleanup(const T *ptr) { delete[] ptr; }
};

template <> struct CleanupTraits<FILE> {
  static void cleanup(FILE *fp) {
    if (fp) fclose(fp);
  }
};

template <> struct CleanupTraits<DIR> {
  static void cleanup(DIR *dp) {
    if (dp) closedir(dp);
  }
};

/**
 * @brief The deletionability of the entry is checked using the @c delete key. Only for pointers/classes!
 *        This check is performed using @c CleanupTraits. It is used by other classes.
 *
 * @tparam T Variable/type.
 */
template <typename T>
concept PointerDeletable = !std::is_void_v<T> && !std::is_array_v<T> && requires(T *p) { CleanupTraits<T>::cleanup(p); };

/**
 * @brief The deletionability of the entry is checked using the @c delete key. Only for arrays!
 *        This check is performed using @c CleanupTraits. It is used by other classes.
 *
 * @tparam T Variable/type.
 */
template <typename T>
concept ArrayDeletable = !std::is_void_v<T> && requires(T *p) { CleanupTraits<T[]>::cleanup(p); };

/**
 * @brief The deletionability of the entry is checked using the @c delete key.
 *        This check is performed using @c CleanupTraits. It is used by other classes.
 *
 * @tparam T Variable/type.
 * @note This concept handles both @c PointerDeletable and @c ArrayDeletable controls.
 */
template <typename T>
concept Deletable = PointerDeletable<T> && ArrayDeletable<T>;

/**
 * @brief A simple class for easily managing asynchronous operations.
 *
 * @code
 * int some_func(int x) {
 *   std::this_thread::sleep_for(std::chrono::milliseconds(100));
 *   return x;
 * }
 *
 * int main(void) {
 *   AsyncManager<int> a_manager;
 *
 *   // Define threads.
 *   a_manager.addProcess(&some_func, 2);
 *   a_manager.addProcess(&some_func, 102);
 *   a_manager.addProcess(&some_func, 84923);
 *
 *   // Start threads.
 *   a_manager.startAll();
 *
 *   // Get results and print.
 *   auto vec = a_manager.getResults();
 *   std::cout << vec << std::endl;
 *
 *   // Results are already get?
 *   if (a_manager.resultsReceived())
 *      std::cout << "Result are already get." << std::endl;
 *
 *   // ONLY FOR std::pair<std::string, bool> LIKE RETURN TYPES //
 *
 *   // Get result and print.
 *   a_manager.finalize();
 *
 *   // getResults() + finalize()
 *   a_manager();
 *
 *   return 0;
 * }
 * @endcode
 *
 * @tparam RetT Return type of target function(s).
 */
template <typename RetT> class AsyncManager {
  std::vector<std::packaged_task<RetT()>> tasks;
  std::vector<std::future<RetT>> futures;
  std::vector<RetT> results;
  bool get = false;

public:
  /// @brief Turn printing on/off.
  bool print = true;

  /**
   * @brief Add new processes.
   *
   * @code
   * class SomeClass {
   * public:
   *   int func(int x) {
   *     std::this_thread::sleep_for(std::chrono::milliseconds(100));
   *     return x;
   *   }
   *
   *   SomeClass() {
   *     AsyncManager<int> a_manager;
   *     a_manager.addProcess(&SomeClass::func, this, 2);
   *     a_manager.addProcess(&SomeClass::func, this, 392);
   *     a_manager.addProcess(&SomeClass::func, this, 950302);
   *     // ...
   *   }
   * };
   * @endcode
   *
   * @param args @c std::thread like input.
   */
  template <typename... Args> void addProcess(Args &&...args) {
    auto bound = std::bind(std::forward<Args>(args)...);
    tasks.emplace_back([bound = std::move(bound)]() mutable { return bound(); });
  }

  /// @brief Start all defined threads.
  void startAll() {
    for (auto &task : tasks) {
      futures.push_back(task.get_future());
      std::thread(std::move(task)).detach();
    }
    tasks.clear();
  }

  /**
   * @brief Get results of threads.
   *
   * @note It waits for unfinished threads.
   * @return List of results.
   */
  std::vector<RetT> getResults() {
    if (!get) {
      std::ranges::for_each(futures, [&](auto &future) { results.push_back(future.get()); });
      get = true;
    }
    return results;
  }

  /// @brief Check the status of the results being provided.
  bool resultsReceived() const { return get; }

  /**
   * @brief Print results. @ref getResults() must have been called beforehand.
   *
   * @note It is only available for return types like @c std::pair<std::string, bool> and similar types.
   */
  bool finalize() const
    requires requires(RetT v) {
      { v.first } -> std::convertible_to<std::string>;
      { v.second } -> std::convertible_to<bool>;
    }
  {
    if (!get) return false;
    std::ostringstream oss;
    bool ret = true;

    for (const auto &res : results) {
      if (!static_cast<bool>(res.second))
        oss << std::string(res.first) << std::endl;
      else if (print)
        std::cout << std::string(res.first) << std::endl;
      ret &= static_cast<bool>(res.second);
    }

    if (!oss.str().empty()) throw Error("{}", oss.str());
    return ret;
  }

  /**
   * @brief Calls @ref getResults() and @ref finalize() .
   *
   * @code
   * AsyncManager<int> a_manager;
   * // ...
   * a_manager();
   * @endcode
   *
   * @note It is only available for return types like @c std::pair<std::string, bool> and similar types.
   */
  bool operator()()
    requires requires(RetT v) {
      { v.first } -> std::convertible_to<std::string>;
      { v.second } -> std::convertible_to<bool>;
    }
  {
    getResults();
    return finalize();
  }
};

/**
 * @brief Checks if the input class can close the needed thing, either as a @c operator() or using the @c close() function.
 *
 * @code
 * // A example valid class structure.
 * class CloserWithFunction {
 *   template <typename>
 *   static constexpr bool always_false = false;
 *
 * public:
 *   template <typename T>
 *   void close(T &var) const noexcept {
 *     if constexpr (std::is_same_v<std::decay_t<T>, int>) {
 *       if (static_cast<int>(var) >= 0) ::close(static_cast<int>(var));
 *     } else if constexpr (std::is_same_v<std::decay_t<T>, FILE*>) {
 *       if (var != nullptr) fclose(var);
 *     } else if constexpr (std::is_same_v<std::decay_t<T>, DIR*>) {
 *       if (var != nullptr) closedir(var);
 *     } else {
 *       static_assert(always_false<T>, "Unsupported input type. Closer is only supports int, FILE* and DIR*");
 *     }
 *   }
 * };
 *
 * // --- OR --- //
 *
 * class CloserWithOperator {
 *   template <typename>
 *   static constexpr bool always_false = false;
 *
 * public:
 *   template <typename T>
 *   void operator()(T &var) const noexcept {
 *     if constexpr (std::is_same_v<std::decay_t<T>, int>) {
 *       if (static_cast<int>(var) >= 0) ::close(static_cast<int>(var));
 *     } else if constexpr (std::is_same_v<std::decay_t<T>, FILE*>) {
 *       if (var != nullptr) fclose(var);
 *     } else if constexpr (std::is_same_v<std::decay_t<T>, DIR*>) {
 *       if (var != nullptr) closedir(var);
 *     } else {
 *       static_assert(always_false<T>, "Unsupported input type. Closer is only supports int, FILE* and DIR*");
 *     }
 *   }
 * };
 *
 * // To see how this concept is used, check out Helper::BasicUniqueFD or Helper::BasicUniqueFP.
 * @endcode
 *
 * @tparam Class Class.
 * @tparam Type Needed type.
 * @see Helper::BasicUniqueFD
 * @see Helper::BasicUniqueFP
 * @note The return type of the desired @c close() function/@c operator() must be @c void.
 */
template <typename Class, typename Type>
concept IsCloser = requires(Class c, Type t) {
  { c(t) } -> std::same_as<void>;
} || requires(Class c, Type t) {
  { c.close(t) } -> std::same_as<void>;
};

/**
 * @brief Checks if the input class can close the needed thing, either as a @c operator() and using the @c close() function.
 *
 * @code
 * // A example valid class structure.
 * class Closer {
 *   template <typename>
 *   static constexpr bool always_false = false;
 *
 * public:
 *   template <typename T>
 *   void close(T &var) const noexcept {
 *     if constexpr (std::is_same_v<std::decay_t<T>, int>) {
 *       if (static_cast<int>(var) >= 0) ::close(static_cast<int>(var));
 *     } else if constexpr (std::is_same_v<std::decay_t<T>, FILE*>) {
 *       if (var != nullptr) fclose(var);
 *     } else if constexpr (std::is_same_v<std::decay_t<T>, DIR*>) {
 *       if (var != nullptr) closedir(var);
 *     } else {
 *       static_assert(always_false<T>, "Unsupported input type. Closer is only supports int, FILE* and DIR*");
 *     }
 *   }
 *
 *   template <typename T>
 *   void operator()(T &var) const noexcept {
 *     close(var);
 *    }
 * };
 *
 * // To see how this concept is used, check out Helper::BasicUniqueFD or Helper::BasicUniqueFP.
 * @endcode
 *
 * @tparam Class Class.
 * @tparam Type Needed type.
 * @see Helper::BasicUniqueFD
 * @see Helper::BasicUniqueFP
 * @note The return type of the desired @c close() function and @c operator() must be @c void.
 */
template <typename Class, typename Type>
concept IsFullCloser = requires(Class c, Type t) {
  { c(t) } -> std::same_as<void>;
  { c.close(t) } -> std::same_as<void>;
};

/**
 * @brief Check the ownership of @c operator() in the closer class.
 *
 * @tparam Class Class.
 * @tparam Type Needed type.
 * @see Helper::BasicUniqueFD
 * @see Helper::BasicUniqueFP
 * @note To see how this concept is used, check out @c Helper::BasicUniqueFD or @c Helper::BasicUniqueFP.
 */
template <typename Class, typename Type>
concept IsCloserWithOperator = requires(Class c, Type t) {
  { c(t) } -> std::same_as<void>;
};

/**
 * @brief Check the ownership of @c close() in the closer class.
 *
 * @tparam Class Class.
 * @tparam Type Needed type.
 * @see Helper::BasicUniqueFD
 * @see Helper::BasicUniqueFP
 * @note To see how this concept is used, check out @c Helper::BasicUniqueFD or @c Helper::BasicUniqueFP.
 */
template <typename Class, typename Type>
concept IsCloserWithFunction = requires(Class c, Type t) {
  { c.close(t) } -> std::same_as<void>;
};

/// @brief Valid file descriptor, file pointer and directory pointer closer class structure. Used by default.
class Closer {
  template <typename> static constexpr bool always_false = false;

public:
  /// @brief Close the input @c fd, @c fp or @c dp.
  template <typename T> void close(T &var) const noexcept {
    if constexpr (std::is_integral_v<std::decay_t<T>>) {
      if (static_cast<int>(var) >= 0) ::close(static_cast<int>(var));
    } else if constexpr (std::is_same_v<std::decay_t<T>, FILE *>) {
      if (var != nullptr) fclose(var);
    } else if constexpr (std::is_same_v<std::decay_t<T>, DIR *>) {
      if (var != nullptr) closedir(var);
    } else {
      static_assert(always_false<T>, "Unsupported input type. Closer is only supports int, FILE* and DIR*");
    }
  }

  /// @brief Close the input @c fd, @c fp or @c dp.
  template <typename T> void operator()(T &var) const noexcept { close(var); }
};

/**
 * @brief A class that helps with easy file descriptor management.
 *
 * @code
 * auto sfd = BasicUniqueFD("file.txt", O_RDONLY);
 * // ... operations
 * @endcode
 *
 * @tparam Closer Closer class. @c Helper::Closer used by default.
 * @see Helper::IsCloser
 * @see Helper::IsCloserWithOperator
 * @see Helper::Closer
 */
template <typename Closer = Helper::Closer>
  requires IsCloser<Closer, int>
class BasicUniqueFD {
  int fd_ = -1, flags_ = 0;
  std::optional<mode_t> mode_ = std::nullopt;
  std::filesystem::path path_;
  [[no_unique_address]] Closer closer_;

public:
  /// @brief Turn on/off synchronization before closing (by object).
  bool syncOnClose = true;

  /// @brief Default constructor, initializes local class variables.
  BasicUniqueFD() = default;

  /// @brief A class that claims the unique property cannot be copied.
  BasicUniqueFD(const BasicUniqueFD &) = delete;

  /// @brief Move constructor.
  BasicUniqueFD(BasicUniqueFD &&other) noexcept
      : fd_(other.fd_), flags_(other.flags_), mode_(other.mode_), path_(other.path_), closer_(other.closer_) {
    other.fd_ = -1;
  }

  /**
   * @brief Constructor that opens the file.
   *
   * @param path File path.
   * @param flags Open flags.
   * @see [open() flags](https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/fcntl.h)
   */
  BasicUniqueFD(const std::filesystem::path &path, int flags) : fd_(::open(path.c_str(), flags)), flags_(flags), path_(path) {}

  /**
   * @brief Constructor that opens the file with mode.
   *
   * @param path File path.
   * @param flags Open flags.
   * @param mode Mode.
   * @see [open() flags](https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/fcntl.h)
   * @see [mode flags](https://github.com/torvalds/linux/blob/master/include/uapi/linux/stat.h)
   */
  BasicUniqueFD(const std::filesystem::path &path, int flags, mode_t mode)
      : fd_(::open(path.c_str(), flags, mode)), flags_(flags), mode_(mode), path_(path) {}

  /**
   * @brief A constructor that takes over the management of an existing file descriptor.
   *
   * @param fd File descriptor.
   * @see Helper::BasicUniqueFD::getOwnership
   */
  explicit BasicUniqueFD(int fd) : fd_(fd) {
    if (fd_ < 0) return;

    int res_flags = ::fcntl(fd_, F_GETFL);
    if (res_flags != -1) flags_ = res_flags;

    struct stat st{};
    if (fstat(fd_, &st) == 0) mode_ = st.st_mode;

    char buf[PATH_MAX];
    std::string proc = "/proc/self/fd/" + std::to_string(fd_);
    ssize_t len = readlink(proc.c_str(), buf, sizeof(buf) - 1);
    if (len != -1) {
      buf[len] = '\0';
      path_ = std::filesystem::path(buf);
    }
  }

  /// @brief The destructor that closes the file descriptor.
  ~BasicUniqueFD() {
    if (fd_ >= 0 && syncOnClose) fsync();

    if constexpr (IsCloserWithOperator<Closer, int>)
      closer_(fd_);
    else
      closer_.close(fd_);
  }

  /**
   * @brief An alternative to @c Helper::BasicUniqueFD(int fd)
   *
   * @param fd File descriptor.
   * @return A BasicUniqueFD object that holds @p fd.
   */
  static BasicUniqueFD getOwnership(int fd) { return BasicUniqueFD(fd); }

  /**
   * @name BasicUniqueFD's function declarations.
   * @brief In-class declarations of functions that work with file descriptors.
   * @{
   */

  int fd() { return fd_; }             ///< Get file descriptor.
  int fd() const { return fd_; }       ///< Get file descriptor.
  int flags() { return flags_; }       ///< Get @c open() flags.
  int flags() const { return flags_; } ///< Get @c open() flags.

  std::optional<mode_t> mode() { return mode_; }       ///< It returns std::nullopt if no mode information is stored!
  std::optional<mode_t> mode() const { return mode_; } ///< It returns std::nullopt if no mode information is stored!

  std::filesystem::path path() { return path_; }       ///< Get file path.
  std::filesystem::path path() const { return path_; } ///< Get file path.

  /**
   * @brief Open.
   * @param withMode Open with mode or not.
   * @return @c *this
   */
  BasicUniqueFD &open(bool withMode = false) {
    if (fd_ >= 0) return *this;
    if (withMode && mode_.has_value())
      fd_ = ::open(path_.c_str(), flags_, *mode_);
    else
      fd_ = ::open(path_.c_str(), flags_);
    return *this;
  }

  /**
   * @brief Open input file.
   *
   * @param path File path.
   * @param flags Open flags.
   * @return @c *this
   * @see [open() flags](https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/fcntl.h)
   */
  BasicUniqueFD &open(const std::filesystem::path &path, int flags) {
    if (fd_ >= 0) return *this;

    path_ = path;
    flags_ = flags;
    fd_ = ::open(path.c_str(), flags);

    return *this;
  }

  /**
   * @brief Open input file with mode.
   *
   * @param path File path.
   * @param flags Open flags.
   * @param mode Mode.
   * @return @c *this
   * @see [open() flags](https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/fcntl.h)
   * @see [mode flags](https://github.com/torvalds/linux/blob/master/include/uapi/linux/stat.h)
   */
  BasicUniqueFD &open(const std::filesystem::path &path, int flags, mode_t mode) {
    if (fd_ >= 0) return *this;

    path_ = path;
    flags_ = flags;
    mode_ = mode;
    fd_ = ::open(path.c_str(), flags, mode);

    return *this;
  }

  /// @brief Class-specific function that reopens the file.
  bool reOpen(bool withMode = false) {
    close();
    if (withMode && mode_.has_value())
      fd_ = ::open(path_.c_str(), flags_, *mode_);
    else
      fd_ = ::open(path_.c_str(), flags_);

    return fd_ >= 0;
  }

  void close() noexcept {
    if (fd_ >= 0 && syncOnClose) fsync();

    if constexpr (IsCloserWithOperator<Closer, int>)
      closer_(fd_);
    else
      closer_.close(fd_);

    fd_ = -1;
  }

  ssize_t read(void *buf, size_t count) const { return ::read(fd_, buf, count); }

  ssize_t write(const void *buf, size_t count) { return ::write(fd_, buf, count); }

  off_t lseek(off_t offset, int whence) { return ::lseek(fd_, offset, whence); }

  int fsync() { return ::fsync(fd_); }

  template <typename... Args> int fcntl(int op, Args... args) { return ::fcntl(fd_, op, args...); }

  template <typename... Args> int ioctl(unsigned long cmd, Args... args) { return ::ioctl(fd_, cmd, args...); }

  int dup() { return ::dup(fd_); }
  int dup2(int newfd) { return ::dup2(fd_, newfd); }

  /** @} */

  /**
   * @name BasicUniqueFD's operator declarations.
   * @brief Definitions of operators required for int-style behavior.
   * @{
   */
  bool operator!() const noexcept { return fd_ < 0; }
  explicit operator bool() const noexcept { return fd_ >= 0; }
  int operator()() noexcept { return fd_; }
  int operator()() const noexcept { return fd_; }

  template <typename T>
    requires std::is_integral_v<T>
  bool operator<(T fd) const noexcept {
    return fd_ < static_cast<int>(fd);
  }

  template <typename T>
    requires std::is_integral_v<T>
  bool operator>(T fd) const noexcept {
    return fd_ > static_cast<int>(fd);
  }

  template <typename T>
    requires std::is_integral_v<T>
  bool operator<=(T fd) const noexcept {
    return fd_ <= static_cast<int>(fd);
  }

  template <typename T>
    requires std::is_integral_v<T>
  bool operator>=(T fd) const noexcept {
    return fd_ >= static_cast<int>(fd);
  }

  template <typename T>
    requires std::is_integral_v<T>
  bool operator==(T fd) const noexcept {
    return fd_ == static_cast<int>(fd);
  }

  template <typename T>
    requires std::is_integral_v<T>
  bool operator!=(T fd) const noexcept {
    return fd_ != static_cast<int>(fd);
  }

  /** @} */

  /// @brief A class that claims the unique property cannot be copied.
  BasicUniqueFD &operator=(const BasicUniqueFD &) = delete;

  /// @brief Mover @c = operator.
  BasicUniqueFD &operator=(BasicUniqueFD &&other) noexcept {
    if (this != &other) {
      close();
      fd_ = other.fd_;
      flags_ = other.flags_;
      mode_ = other.mode_;
      path_ = std::move(other.path_);
      closer_ = std::move(other.closer_);
      other.fd_ = -1;
    }
    return *this;
  }
};

/**
 * @brief A class that helps with easy file pointer management.
 *
 * @code
 * auto sfd = BasicUniqueFP("file.txt", "r");
 * // ... operations
 * @endcode
 *
 * @tparam Closer Closer class. @c Helper::Closer used by default.
 * @see Helper::IsCloser
 * @see Helper::IsCloserWithOperator
 * @see Helper::Closer
 */
template <typename Closer = Helper::Closer>
  requires IsCloser<Closer, FILE *>
class BasicUniqueFP {
  FILE *fp_ = nullptr;
  std::string_view flags_;
  std::filesystem::path path_;
  [[no_unique_address]] Closer closer_;

  std::string_view get_flags(FILE *fp) const {
    if (fp == nullptr) return {};

    int flags = fcntl(fileno(fp), F_GETFL);
    if (flags == -1) return {};

    int acc = flags & O_ACCMODE;

    if (flags & O_APPEND)
      return (acc == O_RDWR) ? "a+" : "a";
    else {
      if (acc == O_RDONLY) return "r";
      if (acc == O_WRONLY) return "w";
      if (acc == O_RDWR) return "r+";
    }

    return {};
  }

public:
  /// @brief Turn on/off flushing before closing (by object).
  bool flushOnClose = true;

  /// @brief Default constructor, initializes local class variables.
  BasicUniqueFP() = default;

  /// @brief A class that claims the unique property cannot be copied.
  BasicUniqueFP(const BasicUniqueFP &) = delete;

  /// @brief Move constructor.
  BasicUniqueFP(BasicUniqueFP &&other) noexcept : fp_(other.fp_), flags_(other.flags_), path_(other.path_), closer_(other.closer_) {
    other.fp_ = nullptr;
  }

  /**
   * @brief Constructor that opens the file.
   *
   * @param path File path.
   * @param flags Open flags.
   * @see [fopen() flags](https://www.man7.org/linux/man-pages/man3/fopen.3.html)
   */
  BasicUniqueFP(const std::filesystem::path &path, const std::string_view &flags)
      : fp_(fopen(path.c_str(), flags.data())), flags_(flags), path_(path) {}

  /**
   * @brief A constructor that takes over the management of an existing file pointer.
   *
   * @param fp File pointer.
   * @see Helper::BasicUniqueFP::getOwnership
   */
  explicit BasicUniqueFP(FILE *fp) : fp_(fp) {
    if (fp_ == nullptr) return;

    int fd_ = fileno(fp_);
    flags_ = get_flags(fp);

    char buf[PATH_MAX];
    std::string proc = "/proc/self/fd/" + std::to_string(fd_);
    ssize_t len = readlink(proc.c_str(), buf, sizeof(buf) - 1);
    if (len != -1) {
      buf[len] = '\0';
      path_ = std::filesystem::path(buf);
    }
  }

  /// @brief The destructor that closes the file pointer.
  ~BasicUniqueFP() {
    if constexpr (IsCloserWithOperator<Closer, FILE *>)
      closer_(fp_);
    else
      closer_.close(fp_);
  }

  /**
   * @brief An alternative to @c Helper::BasicUniqueFP(FILE *fp)
   *
   * @param fp File pointer.
   * @return A BasicUniqueFP object that holds @p fp.
   */
  static BasicUniqueFP getOwnership(FILE *fp) { return BasicUniqueFP(fp); }

  /**
   * @name BasicUniqueFP's function declarations.
   * @brief In-class declarations of functions that work with file pointers.
   * @{
   */

  int rawFd() const noexcept { return fileno(fp_); } ///< Get file descriptor of file pointer.

  FILE *fp() noexcept { return fp_; }
  const FILE *fp() const noexcept { return fp_; }

  std::string_view flags() noexcept { return flags_; }       ///< Get @c fopen() flags.
  std::string_view flags() const noexcept { return flags_; } ///< Get @c fopen() flags.

  /// @brief Get file permissions (mode).
  mode_t mode() const noexcept {
    struct stat st{};
    if (fstat(rawFd(), &st) == 0) return st.st_mode;
    return 0;
  }

  std::filesystem::path path() noexcept { return path_; }       ///< Get file path.
  std::filesystem::path path() const noexcept { return path_; } ///< Get file path.

  /**
   * @brief Open input file.
   *
   * @param path File path.
   * @param flags Open flags.
   * @return @c *this
   * @see [fopen() flags](https://www.man7.org/linux/man-pages/man3/fopen.3.html)
   */
  BasicUniqueFP &open(const std::filesystem::path &path, const std::string_view &flags) noexcept {
    if (fp_ == nullptr) return *this;

    path_ = path;
    flags_ = flags;
    fp_ = fopen(path.c_str(), flags.data());

    return *this;
  }

  /// @brief Class-specific function that reopens the file.
  bool reOpen() {
    close();
    fp_ = fopen(path_.c_str(), flags_.data());

    return fp_ != nullptr;
  }

  void close() noexcept {
    if constexpr (IsCloserWithOperator<Closer, FILE *>)
      closer_(fp_);
    else
      closer_.close(fp_);

    if (fp_ != nullptr && flushOnClose) flush();
    fp_ = nullptr;
  }

  int putc(int c) noexcept { return fputc(c, fp_); }
  int puts(const std::string &s) noexcept { return fputs(s.c_str(), fp_); }

  template <typename... Args> int printf(std::format_string<Args...> fmt, Args &&...args) noexcept {
    std::string end = std::format(fmt, std::forward<Args>(args)...);
    return fprintf(fp_, "%s", end.c_str());
  }

  size_t write(const void *buf, size_t size, size_t nmemb) noexcept { return fwrite(buf, size, nmemb, fp_); }

  int getc() noexcept { return fgetc(fp_); }
  char *gets(char *str, int size) noexcept { return fgets(str, size, fp_); }

  size_t read(void *buf, size_t size, size_t count) noexcept { return fread(buf, size, count, fp_); }

  template <typename... Args> int scanf(const char *fmt, Args &&...args) noexcept { return fscanf(fp_, fmt, args...); }

  int seek(long offset, int whence) noexcept { return fseek(fp_, offset, whence); }
  long tell() noexcept { return ftell(fp_); }

  void rewind() noexcept { return rewind(fp_); }
  void clearerr() noexcept { return ::clearerr(fp_); }

  int eof() noexcept { return feof(fp_); }
  int error() noexcept { return ferror(fp_); }

  int flush() noexcept { return fflush(fp_); }

  /** @} */

  /**
   * @name BasicUniqueFP's operator declarations.
   * @brief Definitions of operators required for pointer-style behavior and usability.
   * @{
   */

  bool operator!() const noexcept { return fp_ == nullptr; }
  explicit operator bool() const noexcept { return fp_ != nullptr; }
  FILE *operator()() noexcept { return fp_; }
  FILE *operator()() const noexcept { return fp_; }

  bool operator==(const FILE *fp) const noexcept { return fp_ == fp; }
  bool operator!=(const FILE *fp) const noexcept { return fp_ != fp; }

  /** @} */

  /// @brief A class that claims the unique property cannot be copied.
  BasicUniqueFP &operator=(const BasicUniqueFP &) = delete;

  /// @brief Mover @c = operator.
  BasicUniqueFP &operator=(BasicUniqueFP &&other) noexcept {
    if (this != &other) {
      close();
      fp_ = other.fp_;
      flags_ = other.flags_;
      path_ = std::move(other.path_);
      closer_ = std::move(other.closer_);
      other.fp_ = nullptr;
    }
    return *this;
  }
};

using UniqueFD = BasicUniqueFD<Closer>;
using UniqueFP = BasicUniqueFP<Closer>;

/**
 * @brief When an object created within a given scope leaves that scope, the corresponding input function is executed.
 *
 * @code
 * int main(void) {
 *   {
 *     int fd = open("file.txt", O_RDONLY);
 *     int* ptr = calloc(100, sizeof(int));
 *     auto scope1 = makeScopeGuard([&] { close(fd); });
 *     auto scope2 = makeScopeGuard([&] { free(ptr); ptr = nullptr; });
 *   }
 *
 *   return 0;
 * }
 * @endcode
 *
 * @tparam F Function.
 * @see [C++ scopes](https://cppreference.com/cpp/language/scope)
 * @see Helper::makeScopeGuard
 * @note Input functions can never be parameterized.
 */
template <std::invocable F> class ScopeGuard {
  F _fn;
  bool _active = true;

public:
  /// @brief Constructor with function input.
  explicit ScopeGuard(F &&fn) : _fn(std::forward<F>(fn)) {}

  /// @brief This class is not copyable.
  ScopeGuard(const ScopeGuard &) = delete;

  /// @brief Move constructor.
  ScopeGuard(ScopeGuard &&other) noexcept : _fn(std::move(other._fn)), _active(other._active) { other.dismiss(); }

  /// @brief Run function if scope guard is active.
  ~ScopeGuard() {
    if (_active) _fn();
  }

  /// @brief Run process now.
  void now() noexcept {
    if (_active) {
      _fn();
      _active = false;
    }
  }

  /// @brief Disable scope guard.
  void dismiss() noexcept { _active = false; }

  /// @brief Release.
  auto release() noexcept {
    dismiss();
    return std::move(_fn);
  }

  /// @brief This class is not copyable.
  ScopeGuard &operator=(const ScopeGuard &) = delete;
};

/**
 * @brief Make scope guard with input function.
 *
 * @tparam F Function reference.
 * @param fn Function.
 * @return Helper::ScopeGuard
 */
template <std::invocable F> [[nodiscard]] static auto makeScopeGuard(F &&fn) noexcept {
  return ScopeGuard<std::decay_t<F>>(std::forward<F>(fn));
}

/// @brief Open file (creates file descriptor).
template <typename... Args>
  requires(sizeof...(Args) >= 2)
inline auto openFd(Args &&...args) noexcept {
  int fd = open(args...);
  return std::pair{fd, makeScopeGuard([fd] {
                     if (fd >= 0) close(fd);
                   })};
}

/// @brief Open file (creates file pointer).
template <typename... Args>
  requires(sizeof...(Args) >= 2)
inline auto openFp(Args &&...args) noexcept {
  FILE *fp = fopen(args...);
  return std::pair{fp, makeScopeGuard([fp] {
                     if (fp != nullptr) fclose(fp);
                   })};
}

/// @brief Open directory.
template <typename... Args>
  requires(sizeof...(Args) >= 1)
inline auto openDir(Args &&...args) noexcept {
  DIR *dir = opendir(args...);
  return std::pair{dir, makeScopeGuard([dir] {
                     if (dir != nullptr) closedir(dir);
                   })};
}

/// @brief Redirect stdout and stderr to /dev/null and block std::cout and std::cerr.
class Silencer {
  std::streambuf *saved_cout = nullptr;
  std::streambuf *saved_cerr = nullptr;
  std::ofstream null_stream;
  int saved_stdout_fd = -1;
  int saved_stderr_fd = -1;
  int null_fd = -1;

public:
  explicit Silencer(bool do_silence = true) {
    if (do_silence) silence();
  }

  ~Silencer() { stop(); }

  Silencer(const Silencer &) = delete;
  Silencer &operator=(const Silencer &) = delete;

  /// @brief Silence the output.
  void silence() {
    if (saved_cout || null_fd != -1) return;
    fflush(stdout);
    fflush(stderr);

    null_stream.open("/dev/null");
    saved_cout = std::cout.rdbuf(null_stream.rdbuf());
    saved_cerr = std::cerr.rdbuf(null_stream.rdbuf());

    null_fd = open("/dev/null", O_WRONLY);
    if (null_fd == -1) return;

    saved_stdout_fd = dup(STDOUT_FILENO);
    saved_stderr_fd = dup(STDERR_FILENO);
    dup2(null_fd, STDOUT_FILENO);
    dup2(null_fd, STDERR_FILENO);
  }

  /// @brief Stop muting.
  void stop() {
    if (!saved_cout && null_fd == -1) return;

    if (saved_cout) {
      std::cout.rdbuf(saved_cout);
      std::cerr.rdbuf(saved_cerr);
      null_stream.close();
      saved_cout = nullptr;
      saved_cerr = nullptr;
    }

    if (null_fd != -1) {
      fflush(stdout);
      fflush(stderr);
      dup2(saved_stdout_fd, STDOUT_FILENO);
      dup2(saved_stderr_fd, STDERR_FILENO);
      close(saved_stdout_fd);
      close(saved_stderr_fd);
      close(null_fd);
      saved_stdout_fd = -1;
      saved_stderr_fd = -1;
      null_fd = -1;
    }
  }
};

} // namespace Helper

#endif // #ifndef LIBHELPER_MANAGEMENT_HPP
