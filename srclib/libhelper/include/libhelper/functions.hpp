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
#include <__filesystem/path.h>
#include <libhelper/macros.hpp>
#include <libhelper/management.hpp>
#include <dirent.h>

namespace Helper {

/**
 * It is checked whether the user ID used is equivalent to AID_ROOT.
 * See external/core/libcutils/include/private/android_filesystem_config.h
 */
bool hasSuperUser();

/**
 * It is checked whether the user ID used is equivalent to AID_SHELL.
 * See external/core/libcutils/include/private/android_filesystem_config.h
 */
bool hasAdbPermissions();

/**
 * Checks whether the file/directory exists.
 */
bool isExists(const std::filesystem::path &entry);

/**
 * Checks whether the file exists.
 */
bool fileIsExists(const std::filesystem::path &file);

/**
 * Checks whether the directory exists.
 */
bool directoryIsExists(const std::filesystem::path &directory);

/**
 * Checks whether the link (symbolic or hard) exists.
 */
bool linkIsExists(const std::filesystem::path &entry);

/**
 * Checks if the entry is a symbolic link.
 */
bool isLink(const std::filesystem::path &entry);

/**
 * Checks if the entry is a symbolic link.
 */
bool isSymbolicLink(const std::filesystem::path &entry);

/**
 * Checks if the entry is a hard link.
 */
bool isHardLink(const std::filesystem::path &entry);

/**
 * Checks whether entry1 is linked to entry2.
 */
bool areLinked(const std::filesystem::path &entry1, const std::filesystem::path &entry2);

/**
 * Writes given text into file.
 * If file does not exist, it is automatically created.
 * Returns true on success.
 */
bool writeFile(const std::filesystem::path &file, std::string_view text);

/**
 * Reads file content into string.
 * On success returns file content.
 * On error returns std::nullopt.
 */
std::optional<std::string> readFile(const std::filesystem::path &file);

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
int64_t fileSize(const std::filesystem::path &file);

/**
 * Read symlinks.
 */
std::string readSymlink(const std::filesystem::path &entry);

/**
 * Compare SHA-256 values SHA-256 of files.
 * Throws Helper::Error on error occurred.
 */
bool sha256Compare(const std::filesystem::path &file1, const std::filesystem::path &file2);

/**
 * Get SHA-256 of file.
 * Throws Helper::Error on error occurred.
 */
std::optional<std::string> sha256Of(const std::filesystem::path &path);

/**
 * Copy file to dest.
 */
bool copyFile(const std::filesystem::path &file, const std::filesystem::path &dest);

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
std::pair<std::string, int> runCommandWithOutput(std::string_view cmd);

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
__printflike(1, 2) std::string format(const char *format, ...);

/**
 * Convert input size to input multiple
 */
template <uint64_t size> int convertTo(const sizeCastTypes type) {
  if (type == KB) return TO_KB(size);
  if (type == MB) return TO_MB(size);
  if (type == GB) return TO_GB(size);
  return static_cast<int>(size);
}

/**
 * Get libhelper library version string.
 */
std::string getLibVersion();

/**
 * Open input path with flags and add to integrity list.
 * And returns file descriptor.
 */
[[nodiscard]] int openAndAddToCloseList(const std::filesystem::path &path, garbageCollector &collector, int flags, mode_t mode = 0000);
/**
 * Open input path with flags and add to integrity list.
 * And returns file pointer.
 */
[[nodiscard]] FILE *openAndAddToCloseList(const std::filesystem::path &path, garbageCollector &collector, const char *mode);
/**
 * Open input directory and add to integrity list.
 * And returns directory pointer.
 */
[[nodiscard]] DIR *openAndAddToCloseList(const std::filesystem::path &path, garbageCollector &collector);

} // namespace Helper

#endif // #ifndef LIBHELPER_FUNCTIONS_HPP
