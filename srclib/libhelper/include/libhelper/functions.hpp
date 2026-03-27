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

#ifndef LIBHELPER_FUNCTIONS_HPP
#define LIBHELPER_FUNCTIONS_HPP

#include <optional>
#include <string>
#include <format>
#include <filesystem>
#include <random>
#include <sys/wait.h>
#include <libhelper/definations.hpp>
#include <libhelper/management.hpp>

namespace Helper {

template <typename __path_type, typename __predicate>
  requires Invocable<__predicate, bool, unsigned int> &&
           (HasCStrFunction<__path_type> || std::is_same_v<std::decay_t<__path_type>, const char *>)
bool __stat_check(__path_type &&__path, __predicate &&__pred) {
  struct stat st{};
  if constexpr (HasCStrFunction<__path_type>) {
    if (stat(__path.c_str(), &st) != 0) return false;
  } else {
    if (stat(__path, &st) != 0) return false;
  }

  return __pred(st.st_mode);
}

template <typename __path_type, typename __predicate>
  requires Invocable<__predicate, bool, unsigned int> &&
           (HasCStrFunction<__path_type> || std::is_same_v<std::decay_t<__path_type>, const char *>)
bool __lstat_check(__path_type &&__path, __predicate &&__pred, bool nlink = false) {
  struct stat st{};
  if constexpr (HasCStrFunction<__path_type>) {
    if (lstat(__path.c_str(), &st) != 0) return false;
  } else {
    if (lstat(__path, &st) != 0) return false;
  }

  if (nlink) return __pred(st.st_nlink);
  return __pred(st.st_mode);
}

/**
 * Checks whether the file/directory exists.
 */
template <typename PathType> bool isExists(PathType &&entry) {
  return __stat_check(std::forward<PathType>(entry), [](unsigned int) { return true; });
}

/**
 * Checks whether the file exists.
 */
template <typename PathType> bool fileIsExists(PathType &&file) {
  return __stat_check(std::forward<PathType>(file), [](mode_t mode) { return S_ISREG(mode); });
}

/**
 * Checks whether the directory exists.
 */
template <typename PathType> bool directoryIsExists(PathType &&path) {
  return __stat_check(std::forward<PathType>(path), [](mode_t mode) { return S_ISDIR(mode); });
}

/**
 * Checks if the entry is a symbolic link.
 */
template <typename PathType> bool isLink(PathType &&entry) {
  return __lstat_check(std::forward<PathType>(entry), [](mode_t mode) { return S_ISLNK(mode); });
}

/**
 * Checks if the entry is a symbolic link.
 */
template <typename PathType> bool isSymbolicLink(PathType &&entry) { return isLink(std::forward<PathType>(entry)); }

/**
 * Checks if the entry is a hard link.
 */
template <typename PathType> bool isHardLink(PathType &&entry) {
  return __lstat_check(std::forward<PathType>(entry), [](nlink_t nlink) { return nlink >= 2; }, true);
}

/**
 * Checks whether the link (symbolic or hard) exists.
 */
template <typename PathType> bool linkIsExists(PathType &&entry) {
  return isLink(std::forward<PathType>(entry)) || isHardLink(std::forward<PathType>(entry));
}

/**
 * Writes given text into file.
 * If file does not exist, it is automatically created.
 * Returns true on success.
 */
template <typename PathType, typename StringType = std::string> bool writeFile(PathType &&file, StringType &&text) {
  std::filesystem::path p(std::forward<PathType>(file));
  std::string str(std::forward<StringType>(text));
  LOGN(HELPER, INFO) << "write input string to " << std::quoted_string(p) << " requested." << std::endl;

  auto fp = UniqueFP(p, "a");
  if (!fp) return false;
  fp.printf("{}", str);

  LOGN(HELPER, INFO) << "write " << std::quoted_string(p) << " successfully." << std::endl;
  return true;
}

/**
 * Reads file content into string.
 * On success, returns file content.
 * On error, returns std::nullopt.
 */
template <typename PathType, typename StringType = std::string>
std::optional<ConstIfCharPointer_t<StringType>> readFile(PathType &&file) {
  std::filesystem::path p(std::forward<PathType>(file));
  LOGN(HELPER, INFO) << "read " << std::quoted_string(p) << " requested." << std::endl;

  auto fp = UniqueFP(p, "r");
  if (!fp) return std::nullopt;

  char buffer[1024];
  std::string str;
  while (fp.gets(buffer, sizeof(buffer)))
    str += buffer;

  LOGN(HELPER, INFO) << "read " << p << " successfull." << std::endl;
  ConstIfCharPointer_t<StringType> res = str.c_str();
  return res;
}

/**
 * Copy file to dest.
 */
template <typename PathType> bool copyFile(PathType &&file, PathType &&dest) {
  std::filesystem::path _file(std::forward<PathType>(file));
  std::filesystem::path _dest(std::forward<PathType>(dest));
  LOGN(HELPER, INFO) << "copy " << std::quoted_string(_file) << " to " << std::quoted_string(_dest) << " requested." << std::endl;

  const auto src_fd = UniqueFD(_file, O_RDONLY);
  if (!src_fd) return false;

  auto dst_fd = UniqueFD(_dest, O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_PERMS);
  if (!dst_fd) return false;

  char buffer[512];
  ssize_t br;
  while ((br = src_fd.read(buffer, 512)) > 0) {
    if (const ssize_t bw = dst_fd.write(buffer, br); bw != br) return false;
  }

  if (br == -1) return false;
  LOGN(HELPER, INFO) << "copy " << std::quoted_string(_file) << " to " << std::quoted_string(_dest) << " successfully." << std::endl;
  return true;
}

/**
 * Create directory.
 */
bool makeDirectory(const std::filesystem::path &path);

/**
 * Create recursive directory.
 */
bool makeRecursiveDirectory(const std::filesystem::path &paths);

/**
 * Create file.
 */
bool createFile(const std::filesystem::path &path);

/**
 * Symlink entry1 to entry2.
 */
bool createSymlink(const std::filesystem::path &entry1, const std::filesystem::path &entry2);

/**
 * Remove file or empty directory.
 */
bool eraseEntry(const std::filesystem::path &entry);

/**
 * Remove directory and all directory contents recursively.
 */
bool eraseDirectoryRecursive(const std::filesystem::path &directory);

/**
 * Get file size.
 */
template <typename ReturnType = int64_t>
  requires std::is_integral_v<ReturnType>
ReturnType fileSize(const std::filesystem::path &file) {
  LOGN(HELPER, INFO) << "get file size request: " << std::quoted(file.string()) << std::endl;
  struct stat st{};
  if (stat(file.c_str(), &st) != 0) return -1;
  return static_cast<ReturnType>(st.st_size);
}

/**
 * Read symlinks.
 */
std::string readSymlink(const std::filesystem::path &entry);

/**
 * Checks whether entry1 is linked to entry2.
 */
inline bool areLinked(const std::filesystem::path &entry1, const std::filesystem::path &entry2) {
  auto &&st1 = isSymbolicLink(entry1) ? readSymlink(entry1) : entry1.string();
  auto &&st2 = isSymbolicLink(entry2) ? readSymlink(entry2) : entry2.string();

  return st1 == st2;
}

/**
 * Compare SHA-256 values of files.
 * Throws Helper::Error on error occurred.
 */
bool sha256Compare(const std::filesystem::path &file1, const std::filesystem::path &file2);

/**
 * Get SHA-256 of file.
 * Throws Helper::Error on error occurred.
 */
std::optional<std::string> sha256Of(const std::filesystem::path &path);

/**
 * Run shell command.
 */
bool runCommand(const std::string& cmd);

/**
 * Shows message and asks for y/N from user.
 */
bool confirmPropt(const std::string &message, int maxTries = 10);

/**
 * Change file permissions.
 */
bool changeMode(const std::filesystem::path &file, mode_t mode);

/**
 * Change file owner (user ID and group ID).
 */
bool changeOwner(const std::filesystem::path &file, uid_t uid, gid_t gid);

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
template <typename ExitCodeType = int>
  requires std::is_integral_v<ExitCodeType>
std::pair<std::string, ExitCodeType> runCommandWithOutput(const std::string &cmd) {
  LOGN(HELPER, INFO) << "run command and catch out request: " << cmd << std::endl;

  int pipefd[2];
  if (pipe(pipefd) < 0) return {{}, (ExitCodeType)-1};

  auto closePipe = makeScopeGuard([&] {
    close(pipefd[0]);
    close(pipefd[1]);
  });

  const pid_t pid = fork();
  if (pid < 0) return {{}, (ExitCodeType)-1};

  if (pid == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);

    const std::array<const char *, 4> args = {
#ifdef __ANDROID__
        "/system/bin/sh",
#else
        "/bin/sh",
#endif
        "-c", cmd.data(), nullptr};
    execvp(args[0], const_cast<char *const *>(args.data()));
    _exit(127);
  }

  close(pipefd[1]);
  auto closeWrite = makeScopeGuard([&] { close(pipefd[0]); });
  closePipe.dismiss();

  std::string output;
  auto buffer = std::make_unique<char[]>(1024);

  ssize_t n;
  while ((n = read(pipefd[0], buffer.get(), 1024)) > 0)
    output.append(buffer.get(), n);

  ExitCodeType status = 0;
  if (waitpid(pid, &status, 0) < 0) return {output, (ExitCodeType)-1};

  return {output, (ExitCodeType)(WIFEXITED(status) ? WEXITSTATUS(status) : -1)};
}

/**
 * Joins base path with relative path and returns result.
 */
std::filesystem::path pathJoin(std::filesystem::path base, const std::filesystem::path &relative);

/**
 * Get the filename part of given path.
 */
std::filesystem::path pathBasename(const std::filesystem::path &entry);

/**
 * Get the directory part of given path.
 */
std::filesystem::path pathDirname(const std::filesystem::path &entry);

/**
 * Get random offset depending on size and bufferSize.
 */
template <typename Ret = uint64_t, typename T = Ret>
  requires std::is_integral_v<Ret> && std::is_integral_v<T> && std::is_unsigned_v<Ret> && std::is_unsigned_v<T>
Ret getRandomOffset(T &&size, T &&bufferSize) {
  if (size <= bufferSize) return 0;
  const Ret maxOffset = size - bufferSize;
  static std::mt19937_64 generator(std::random_device{}());
  std::uniform_int_distribution<T> distribution(0, maxOffset - 1);
  return static_cast<Ret>(distribution(generator));
}

/**
 * Convert input size to input multiple.
 */
template <typename Ret = int, typename SizeType = uint64_t>
  requires std::is_integral_v<Ret> && std::is_integral_v<SizeType>
Ret convertTo(SizeType size, sizeCastTypes type) {
  if (type == KB) return TO_KB(size);
  if (type == MB) return TO_MB(size);
  if (type == GB) return TO_GB(size);
  return static_cast<SizeType>(size);
}

/**
 * Convert input multiple variable to string.
 */
std::string multipleToString(sizeCastTypes type);

/**
 * Format it input and return as std::string.
 */
template <typename... Args> std::string format(std::format_string<Args...> fmt, Args &&...args) {
  const std::string message = std::format(fmt, std::forward<Args>(args)...);
  return message;
}

/**
 * Convert input size to input multiple
 */
template <uint64_t size> int convertTo(const sizeCastTypes &type) {
  if (type == KB) return TO_KB(size);
  if (type == MB) return TO_MB(size);
  if (type == GB) return TO_GB(size);
  return static_cast<int>(size);
}

/**
 * Get libhelper library version string.
 */
std::string getLibVersion();
} // namespace Helper

#endif // #ifndef LIBHELPER_FUNCTIONS_HPP
