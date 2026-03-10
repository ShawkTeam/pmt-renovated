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

#ifndef LIBHELPER_MANAGEMENT_HPP
#define LIBHELPER_MANAGEMENT_HPP

#if __cplusplus < 202002L
#error "libhelper/management.hpp is requires C++20 or higher C++ standarts."
#endif

#include <type_traits>
#include <vector>
#include <functional>
#include <future>
#include <sstream>
#include <iostream>
#include <ranges>
#include <libhelper/error.hpp>
#include <filesystem>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

namespace Helper {
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

template <typename T>
concept Deletable = !std::is_void_v<T> && !std::is_array_v<T> && requires(T *p) { CleanupTraits<T>::cleanup(p); };

template <typename T>
concept ArrayDeletable = !std::is_void_v<T> && requires(T *p) { CleanupTraits<T[]>::cleanup(p); };

template <typename __return_type> class AsyncManager {
  std::vector<std::future<__return_type>> futures;
  std::vector<__return_type> results;
  bool get = false;

public:
  template <typename... Args> void addProcess(Args &&...args) {
    futures.push_back(std::async(std::launch::async, std::forward<Args>(args)...));
  }

  std::vector<__return_type> getResults() {
    if (!get) {
      std::ranges::for_each(futures, [&](auto &future) { results.push_back(future.get()); });
      get = true;
    }
    return results;
  }

  bool resultsReceived() const { return get; }

  bool finalize() const
    requires requires(__return_type v) {
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
      else
        std::cout << std::string(res.first) << std::endl;
      ret &= static_cast<bool>(res.second);
    }

    if (!oss.str().empty()) throw Error("%s", oss.str().c_str());
    return ret;
  }

  bool operator()()
    requires requires(__return_type v) {
      { v.first } -> std::convertible_to<std::string>;
      { v.second } -> std::convertible_to<bool>;
    }
  {
    getResults();
    return finalize();
  }
};

template <typename T>
concept IsCloser = requires(T t, int fd) {
  { t(fd) } -> std::same_as<void>;
} || requires(T t, int fd) {
  { t.close(fd) } -> std::same_as<void>;
};

template <typename T>
concept CloserHasOperator = requires(T t, int fd) {
  { t(fd) } -> std::same_as<void>;
};

class DefaultCloser {
public:
  void operator()(int fd) const noexcept {
    if (fd >= 0) ::close(fd);
  }

  void close(int fd) const noexcept {
    if (fd >= 0) ::close(fd);
  }
};

template <typename Closer> class BasicUniqueFD {
  int fd_ = -1, flags_ = 0;
  std::optional<mode_t> mode_ = std::nullopt;
  std::filesystem::path path_;
  [[no_unique_address]] Closer closer_;

public:
  BasicUniqueFD() = default;
  BasicUniqueFD(const BasicUniqueFD &) = delete;
  BasicUniqueFD(BasicUniqueFD &&other) noexcept : fd_(other.fd_), closer_(other.closer_) { other.fd_ = -1; }
  BasicUniqueFD(std::filesystem::path path, int flags) : fd_(::open(path.c_str(), flags)), flags_(flags), path_(std::move(path)) {}
  BasicUniqueFD(std::filesystem::path path, int flags, mode_t mode)
      : fd_(::open(path.c_str(), flags, mode)), flags_(flags), mode_(mode), path_(std::move(path)) {}

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

  ~BasicUniqueFD() {
    if constexpr (CloserHasOperator<Closer>)
      closer_(fd_);
    else
      closer_.close(fd_);
  }

  static BasicUniqueFD getOwnership(int fd) { return BasicUniqueFD(fd); }

  int fd() { return fd_; }
  int fd() const { return fd_; }
  int flags() { return flags_; }
  int flags() const { return flags_; }

  std::optional<mode_t> mode() { return mode_; }
  std::optional<mode_t> mode() const { return mode_; }

  std::filesystem::path path() { return path_; }
  std::filesystem::path path() const { return path_; }

  BasicUniqueFD &open(bool withMode = false) {
    if (withMode && mode_.has_value())
      fd_ = ::open(path_.c_str(), flags_, *mode_);
    else
      fd_ = ::open(path_.c_str(), flags_);
    return *this;
  }

  BasicUniqueFD &open(const std::filesystem::path &path, int flags) {
    path_ = path;
    flags_ = flags;
    fd_ = ::open(path.c_str(), flags);

    return *this;
  }

  BasicUniqueFD &open(const std::filesystem::path &path, int flags, mode_t mode) {
    path_ = path;
    flags_ = flags;
    mode_ = mode;
    fd_ = ::open(path.c_str(), flags, mode);

    return *this;
  }

  bool reOpen(bool withMode = false) {
    close();
    if (withMode && mode_.has_value())
      fd_ = ::open(path_.c_str(), flags_, *mode_);
    else
      fd_ = ::open(path_.c_str(), flags_);

    return fd_ >= 0;
  }

  void close() noexcept {
    if (fd_ >= 0) {
      if constexpr (CloserHasOperator<Closer>)
        closer_(fd_);
      else
        closer_.close(fd_);
      fd_ = -1;
    }
  }

  ssize_t read(void *buf, size_t count) const { return ::read(fd_, buf, count); }

  ssize_t write(const void *buf, size_t count) { return ::write(fd_, buf, count); }

  off_t lseek(off_t offset, int whence) { return ::lseek(fd_, offset, whence); }

  int fsync() { return ::fsync(fd_); }

  template <typename... Args> int fcntl(int op, Args... args) { return ::fcntl(fd_, op, args...); }

  template <typename... Args> int ioctl(unsigned long cmd, Args... args) { return ::ioctl(fd_, cmd, args...); }

  int dup() { return ::dup(fd_); }
  int dup2(int newfd) { return ::dup2(fd_, newfd); }

  bool operator!() const noexcept { return fd_ < 0; }
  explicit operator bool() const noexcept { return fd_ >= 0; }
  int operator()() noexcept { return fd_; }
  int operator()() const noexcept { return fd_; }

  template <typename T>
    requires std::is_integral_v<T>
  bool operator<(T fd) const noexcept {
    return fd_ < fd;
  }

  template <typename T>
    requires std::is_integral_v<T>
  bool operator>(T fd) const noexcept {
    return fd_ > fd;
  }

  template <typename T>
    requires std::is_integral_v<T>
  bool operator<=(T fd) const noexcept {
    return fd_ <= fd;
  }

  template <typename T>
    requires std::is_integral_v<T>
  bool operator>=(T fd) const noexcept {
    return fd_ >= fd;
  }

  template <typename T>
    requires std::is_integral_v<T>
  bool operator==(T fd) const noexcept {
    return fd_ == fd;
  }

  template <typename T>
    requires std::is_integral_v<T>
  bool operator!=(T fd) const noexcept {
    return fd_ != fd;
  }

  BasicUniqueFD &operator=(const BasicUniqueFD &) = delete;
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

using UniqueFD = BasicUniqueFD<DefaultCloser>;

class garbageCollector {
  std::vector<std::function<void()>> __cleaners;

public:
  garbageCollector() = default;
  garbageCollector(const garbageCollector &) = delete;

  ~garbageCollector() {
    for (auto &__cleaner : std::ranges::reverse_view(__cleaners))
      __cleaner();
  }

  template <typename T> T *allocateArray(const size_t size) {
    T *ptr = new T[size];
    delArrayAfterProgress(ptr);
    return ptr;
  }

  template <Deletable T> void delAfterProgress(T *ptr) {
    static_assert(!std::is_array_v<T>, "Use delArrayAfterProgress for arrays");
    __cleaners.push_back([ptr] { CleanupTraits<T>::cleanup(ptr); });
  }

  template <ArrayDeletable T> void delArrayAfterProgress(T *ptr) {
    __cleaners.push_back([ptr] { CleanupTraits<T[]>::cleanup(ptr); });
  }

  void delFileAfterProgress(const std::filesystem::path &path) {
    __cleaners.emplace_back([path] { std::filesystem::remove(path); });
  }

  void closeAfterProgress(FILE *fp) {
    __cleaners.emplace_back([fp] { CleanupTraits<FILE>::cleanup(fp); });
  }

  void closeAfterProgress(DIR *dp) {
    __cleaners.emplace_back([dp] { CleanupTraits<DIR>::cleanup(dp); });
  }

  void closeAfterProgress(int fd) {
    __cleaners.emplace_back([fd] {
      if (fd >= 0) close(fd);
    });
  }

  garbageCollector &operator=(const garbageCollector &) = delete;
};

class Silencer {
  int saved_stdout = -1, saved_stderr = -1, dev_null = -1;

public:
  explicit Silencer(bool silence = true) {
    if (silence) silenceAgain();
  }
  ~Silencer() {
    if (saved_stdout != -1 && dev_null != -1) stop();
  }

  void stop() {
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

  void silenceAgain() {
    if (saved_stdout != -1 && saved_stderr != -1 && dev_null != -1) return;
    fflush(stdout);
    fflush(stderr);
    saved_stdout = dup(STDOUT_FILENO);
    saved_stderr = dup(STDERR_FILENO);
    dev_null = open("/dev/null", O_WRONLY);
    dup2(dev_null, STDOUT_FILENO);
    dup2(dev_null, STDERR_FILENO);
  }
};

} // namespace Helper

#endif // #ifndef LIBHELPER_MANAGEMENT_HPP
