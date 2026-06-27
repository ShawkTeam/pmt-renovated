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
 * @file functions.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Functions of @c libhelper.
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

/** @cond */
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
/** @endcond */

/**
 * @brief Checks whether the file/directory exists.
 * @param entry File/directory path.
 */
template <typename PathType> bool isExists(PathType &&entry) {
  return __stat_check(std::forward<PathType>(entry), [](unsigned int) { return true; });
}

/**
 * @brief Checks whether the file exists.
 * @param file File path.
 */
template <typename PathType> bool fileIsExists(PathType &&file) {
  return __stat_check(std::forward<PathType>(file), [](mode_t mode) { return S_ISREG(mode); });
}

/**
 * @brief Checks whether the directory exists.
 * @param path Directory path.
 */
template <typename PathType> bool directoryIsExists(PathType &&path) {
  return __stat_check(std::forward<PathType>(path), [](mode_t mode) { return S_ISDIR(mode); });
}

/**
 * @brief Checks if the entry is a symbolic link.
 * @param entry File/directory path.
 */
template <typename PathType> bool isLink(PathType &&entry) {
  return __lstat_check(std::forward<PathType>(entry), [](mode_t mode) { return S_ISLNK(mode); });
}

/**
 * @brief Checks if the entry is a symbolic link.
 * @param entry Fİle/directory path.
 */
template <typename PathType> bool isSymbolicLink(PathType &&entry) { return isLink(std::forward<PathType>(entry)); }

/**
 * @brief Checks if the entry is a hard link.
 * @param entry File/directory path.
 */
template <typename PathType> bool isHardLink(PathType &&entry) {
  return __lstat_check(std::forward<PathType>(entry), [](nlink_t nlink) { return nlink >= 2; }, true);
}

/**
 * @brief Checks whether the link (symbolic or hard) exists.
 * @param entry File/directory path.
 */
template <typename PathType> bool linkIsExists(PathType &&entry) {
  return isLink(std::forward<PathType>(entry)) || isHardLink(std::forward<PathType>(entry));
}

/**
 * @brief Writes given text into file. If file does not exist, it is automatically created.
 * @tparam StringType String type (default is @c std::string ).
 * @param file File path.
 * @param text Text.
 */
template <typename PathType, typename StringType = std::string> bool writeFile(PathType &&file, StringType &&text) {
  std::filesystem::path p(std::forward<PathType>(file));
  std::string str(std::forward<StringType>(text));
  Log::info("Writing input string to {}.", std::quoted_string(p));

  auto fp = UniqueFP(p, "a");
  if (!fp) return false;
  fp.printf("{}", str);
  return true;
}

/**
 *
 * On success, returns file content.
 * On error, returns std::nullopt.
 */
/**
 * @brief Reads file content into string.
 * @tparam StringType String type (default is @c std::string ).
 * @param file File path.
 * @retval std::nullopt Error occurred.
 * @retval StringType File content.
 */
template <typename PathType, typename StringType = std::string>
std::optional<ConstIfCharPointer_t<StringType>> readFile(PathType &&file) {
  std::filesystem::path p(std::forward<PathType>(file));
  Log::info("Reading file content from {}.", std::quoted_string(p));

  auto fp = UniqueFP(p, "r");
  if (!fp) return std::nullopt;

  char buffer[1024];
  std::string str;
  while (fp.gets(buffer, sizeof(buffer)))
    str += buffer;

  ConstIfCharPointer_t<StringType> res = str.c_str();
  return res;
}

/**
 * @brief Copy file to destination.
 * @param file File path.
 * @param dest Destination.
 */
template <typename PathType> bool copyFile(PathType &&file, PathType &&dest) {
  std::filesystem::path _file(std::forward<PathType>(file));
  std::filesystem::path _dest(std::forward<PathType>(dest));
  Log::info("Copying file from {} to {}.", std::quoted_string(_file), std::quoted_string(_dest));

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
  return true;
}

/// @brief Create directory.
bool makeDirectory(const std::filesystem::path &path);

/// @brief Create recursive directory.
bool makeRecursiveDirectory(const std::filesystem::path &paths);

/// @brief Create file.
bool createFile(const std::filesystem::path &path);

/// @brief Symlink entries.
bool createSymlink(const std::filesystem::path &entry1, const std::filesystem::path &entry2);

/// @brief Remove file or empty directory.
bool eraseEntry(const std::filesystem::path &entry);

/// @brief Remove directory and all directory contents recursively.
bool eraseDirectoryRecursive(const std::filesystem::path &directory);

/**
 * @brief Get file size.
 * @tparam ReturnType Return type (default is @c int64_t ).
 * @param file File path.
 */
template <typename ReturnType = int64_t>
  requires std::is_integral_v<ReturnType>
ReturnType fileSize(const std::filesystem::path &file) {
  Log::info("Getting file size of {}.", file.string());
  struct stat st{};
  if (stat(file.c_str(), &st) != 0) return -1;
  return static_cast<ReturnType>(st.st_size);
}

/// @brief Read symlinks.
std::string readSymlink(const std::filesystem::path &entry);

/// @brief It checks if the two entries are linked.
inline bool areLinked(const std::filesystem::path &entry1, const std::filesystem::path &entry2) {
  auto &&st1 = isSymbolicLink(entry1) ? readSymlink(entry1) : entry1.string();
  auto &&st2 = isSymbolicLink(entry2) ? readSymlink(entry2) : entry2.string();

  return st1 == st2;
}

/**
 * @brief Compare SHA-256 values of files.
 * @param file1 First file path.
 * @param file2 Second file path.
 * @throws Helper::Error
 */
bool sha256Compare(const std::filesystem::path &file1, const std::filesystem::path &file2);

/**
 * @brief Get SHA-256 of file.
 * @param path File path.
 * @throws Helper::Error
 */
std::optional<std::string> sha256Of(const std::filesystem::path &path);

/// @brief Run shell command.
bool runCommand(const std::string &cmd);

/// Shows message and asks for y/N from user.
bool confirmPropt(const std::string &message, int maxTries = 10);

/// @brief Changes file permissions.
bool changeMode(const std::filesystem::path &file, mode_t mode);

/// @brief Change file owner (user ID and group ID).
bool changeOwner(const std::filesystem::path &file, uid_t uid, gid_t gid);

/**
 * @brief Get current working directory as string.
 * @retval "Empty String" An error occurred.
 */
std::string currentWorkingDirectory();

/**
 * @brief Get current date as string (format: YYYY-MM-DD).
 * @retval "Empty String" An error occurred.
 */
std::string currentDate();

/**
 * @brief Get current time as string (format: HH:MM:SS).
 * @retval "Empty String" An error occurred.
 */
std::string currentTime();

/// @brief Run shell command return output as string.
template <typename ExitCodeType = int>
  requires std::is_integral_v<ExitCodeType>
std::pair<std::string, ExitCodeType> runCommandWithOutput(const std::string &cmd) {
  Log::info("Running command and catch output: {}.", cmd);

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

/// @brief Joins base path with relative path and returns result.
std::filesystem::path pathJoin(std::filesystem::path base, const std::filesystem::path &relative);

/// @briefGet the filename part of given path.
std::filesystem::path pathBasename(const std::filesystem::path &entry);

/// @brief Get the directory part of given path.
std::filesystem::path pathDirname(const std::filesystem::path &entry);

/// @brief Get random offset depending on size and bufferSize.
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
 * @brief Convert input size to input multiple.
 * @tparam Ret Return type.
 * @tparam SizeType Input size type
 * @param size Size.
 * @param type Cast type.
 */
template <typename Ret = int, typename SizeType = uint64_t>
  requires std::is_integral_v<Ret> && std::is_integral_v<SizeType>
Ret convertTo(SizeType size, sizeCastTypes type) {
  if (type == KB) return TO_KB(size);
  if (type == MB) return TO_MB(size);
  if (type == GB) return TO_GB(size);
  return static_cast<SizeType>(size);
}

/// @brief Convert input multiple variable to string.
std::string multipleToString(sizeCastTypes type);

/// @brief Format it input and return as @c std::string.
template <typename... Args> std::string format(std::format_string<Args...> fmt, Args &&...args) {
  const std::string message = std::format(fmt, std::forward<Args>(args)...);
  return message;
}

/**
 * @brief Convert input size to input multiple
 * @tparam size Size.
 * @param type Cast type.
 */
template <uint64_t size> int convertTo(const sizeCastTypes &type) {
  if (type == KB) return TO_KB(size);
  if (type == MB) return TO_MB(size);
  if (type == GB) return TO_GB(size);
  return static_cast<int>(size);
}

/// @brief Get @c libhelper library version string.
std::string getLibVersion();
} // namespace Helper

#endif // #ifndef LIBHELPER_FUNCTIONS_HPP
