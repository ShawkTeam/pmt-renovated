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

#include <cerrno>
#include <iomanip>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <libhelper/functions.hpp>
#include <libhelper/management.hpp>

namespace Helper {

bool makeDirectory(const std::filesystem::path &path) {
  if (isExists(path)) return false;
  LOGN(HELPER, INFO) << "trying make directory: " << std::quoted(path.string()) << std::endl;
  return mkdir(path.c_str(), DEFAULT_DIR_PERMS) == 0;
}

bool makeRecursiveDirectory(const std::filesystem::path &paths) {
  LOGN(HELPER, INFO) << "make recursive directory requested: " << std::quoted(paths.string()) << std::endl;

  char tmp[PATH_MAX];

  snprintf(tmp, sizeof(tmp), "%s", paths.c_str());
  if (const size_t len = strlen(tmp); tmp[len - 1] == '/') tmp[len - 1] = '\0';

  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (access(tmp, F_OK) != 0) {
        if (mkdir(tmp, DEFAULT_DIR_PERMS) != 0 && errno != EEXIST) return false;
      }
      *p = '/';
    }
  }

  if (access(tmp, F_OK) != 0) {
    if (mkdir(tmp, DEFAULT_DIR_PERMS) != 0 && errno != EEXIST) return false;
  }

  LOGN(HELPER, INFO) << std::quoted(paths.string()) << " successfully created." << std::endl;
  return true;
}

bool createFile(const std::filesystem::path &path) {
  LOGN(HELPER, INFO) << "create file request: " << std::quoted(path.string()) << std::endl;

  if (isExists(path)) return false;

  const auto fd = UniqueFD(path.c_str(), O_RDONLY | O_CREAT, DEFAULT_FILE_PERMS);
  if (!fd) return false;

  LOGN(HELPER, INFO) << "create file " << std::quoted(path.string()) << " successfull." << std::endl;
  return true;
}

bool createSymlink(const std::filesystem::path &entry1, const std::filesystem::path &entry2) {
  LOGN(HELPER, INFO) << "symlink " << std::quoted(entry1.string()) << " to " << std::quoted(entry2.string()) << " requested."
                     << std::endl;
  if (const int ret = symlink(entry1.c_str(), entry2.c_str()); ret != 0) return false;

  LOGN(HELPER, INFO) << std::quoted(entry1.string()) << " symlinked to " << std::quoted(entry2.string()) << " successfully."
                     << std::endl;
  return true;
}

bool eraseEntry(const std::filesystem::path &entry) {
  LOGN(HELPER, INFO) << "erase " << std::quoted(entry.string()) << " requested." << std::endl;
  if (const int ret = remove(entry.c_str()); ret != 0) return false;

  LOGN(HELPER, INFO) << std::quoted(entry.string()) << " erased successfully." << std::endl;
  return true;
}

bool eraseDirectoryRecursive(const std::filesystem::path &directory) {
  LOGN(HELPER, INFO) << "erase recursive requested: " << std::quoted(directory.string()) << std::endl;
  struct stat buf{};
  dirent *entry;

  auto [dir, guard] = openDir(directory);
  if (dir == nullptr) return false;

  while ((entry = readdir(dir)) != nullptr) {
    char fullpath[PATH_MAX];

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

    snprintf(fullpath, sizeof(fullpath), "%s/%s", directory.c_str(), entry->d_name);

    if (lstat(fullpath, &buf) == -1) return false;

    if (S_ISDIR(buf.st_mode)) {
      if (!eraseDirectoryRecursive(fullpath)) return false;
    } else if (S_ISREG(buf.st_mode)) {
      if (!eraseEntry(fullpath)) return false;
    } else {
      if (unlink(fullpath) == -1) return false;
    }
  }

  if (rmdir(directory.c_str()) == -1) return false;

  LOGN(HELPER, INFO) << std::quoted(directory.string()) << " successfully erased." << std::endl;
  return true;
}

std::string readSymlink(const std::filesystem::path &entry) {
  LOGN(HELPER, INFO) << "read symlink request: " << std::quoted(entry.string()) << std::endl;

  char target[PATH_MAX];
  const ssize_t len = readlink(entry.c_str(), target, (sizeof(target) - 1));
  if (len == -1) return entry.c_str();

  target[len] = '\0';
  LOGN(HELPER, INFO) << std::quoted(entry.string()) << " is symlink to " << std::quoted(target) << std::endl;
  return target;
}
} // namespace Helper
