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
  Log::info("Trying make directory: {}.", std::quoted_string(path));
  return mkdir(path.c_str(), DEFAULT_DIR_PERMS) == 0;
}

bool makeRecursiveDirectory(const std::filesystem::path &paths) {
  Log::info("Trying make recursive directory: {}.", std::quoted_string(paths));

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

  Log::info("{} successfully created.", std::quoted_string(paths));
  return true;
}

bool createFile(const std::filesystem::path &path) {
  Log::info("Trying create file: {}.", std::quoted_string(path));

  if (isExists(path)) return false;
  const auto fd = UniqueFD(path.c_str(), O_RDONLY | O_CREAT, DEFAULT_FILE_PERMS);
  if (!fd) return false;

  Log::info("{} successfully created.", std::quoted_string(path));
  return true;
}

bool createSymlink(const std::filesystem::path &entry1, const std::filesystem::path &entry2) {
  Log::info("Trying symlink {} to {}.", std::quoted_string(entry1), std::quoted_string(entry2));

  if (const int ret = symlink(entry1.c_str(), entry2.c_str()); ret != 0) return false;
  Log::info("{} successfully symlinked to {}.", std::quoted_string(entry1), std::quoted_string(entry2));
  return true;
}

bool eraseEntry(const std::filesystem::path &entry) {
  Log::info("Trying to remove {}.", std::quoted_string(entry));
  if (const int ret = remove(entry.c_str()); ret != 0) return false;

  Log::info("{} removed.", std::quoted_string(entry));
  return true;
}

bool eraseDirectoryRecursive(const std::filesystem::path &directory) {
  Log::info("Trying to remove {} recursively.", std::quoted_string(directory));
  struct stat buf{};
  dirent *entry;

  auto [dir, guard] = openDir(directory.c_str());
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

  Log::info("{} removed recursively.", std::quoted_string(directory));
  return true;
}

std::string readSymlink(const std::filesystem::path &entry) {
  Log::info("Trying to read {} symlink.", std::quoted_string(entry));

  char target[PATH_MAX];
  const ssize_t len = readlink(entry.c_str(), target, (sizeof(target) - 1));
  if (len == -1) return entry.c_str();

  target[len] = '\0';
  Log::info("{} symlink read.", std::quoted_string(entry));
  return target;
}

} // namespace Helper
