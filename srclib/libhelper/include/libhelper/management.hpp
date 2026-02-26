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
      std::ranges::for_each(futures, [&](auto& future) {
        results.push_back(future.get());
      });
      get = true;
    }
    return results;
  }

  bool resultsReceived() const { return get; }

  bool finalize() const requires std::same_as<std::pair<std::string, bool>, __return_type> {
    if (!get) return false;
    std::ostringstream oss;
    bool ret = true;

    for (const auto& res : results) {
      if (!res.second) oss << res.first << std::endl;
      else std::cout << res.first << std::endl;
      ret &= res.second;
    }

    if (!oss.str().empty()) throw Error("%s", oss.str().c_str());
    return ret;
  }

  bool operator()() {
    getResults();
    return finalize();
  }
};

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
    __cleaners.emplace_back([fd] { if (fd >= 0) close(fd); });
  }

  garbageCollector &operator=(const garbageCollector &) = delete;
};

class Silencer {
  int saved_stdout = -1, saved_stderr = -1, dev_null = -1;

public:
  Silencer() { silenceAgain(); }
  ~Silencer() { if (saved_stdout != -1 && dev_null != -1) stop(); }

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
