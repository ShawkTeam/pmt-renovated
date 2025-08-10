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

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <libhelper/lib.hpp>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace Helper {
bool writeFile(const std::string_view file, const std::string_view text) {
  LOGN(HELPER, INFO) << "write \"" << text << "\" to " << file << "requested."
                     << std::endl;
  garbageCollector collector;

  FILE *fp = openAndAddToCloseList(file, collector, "a");
  if (fp == nullptr) return false;
  fprintf(fp, "%s", text.data());

  LOGN(HELPER, INFO) << "write " << file << " successfully." << std::endl;
  return true;
}

std::optional<std::string> readFile(const std::string_view file) {
  LOGN(HELPER, INFO) << "read " << file << " requested." << std::endl;
  garbageCollector collector;

  FILE *fp = openAndAddToCloseList(file, collector, "r");
  if (fp == nullptr) return std::nullopt;

  char buffer[1024];
  std::string str;
  while (fgets(buffer, sizeof(buffer), fp))
    str += buffer;

  LOGN(HELPER, INFO) << "read " << file << " successfully, read text: \"" << str
                     << "\"" << std::endl;
  return str;
}

bool copyFile(const std::string_view file, const std::string_view dest) {
  LOGN(HELPER, INFO) << "copy " << file << " to " << dest << " requested."
                     << std::endl;
  garbageCollector collector;

  const int src_fd = openAndAddToCloseList(file.data(), collector, O_RDONLY);
  if (src_fd == -1) return false;

  const int dst_fd = openAndAddToCloseList(
      dest.data(), collector, O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_PERMS);
  if (dst_fd == -1) return false;

  char buffer[512];
  ssize_t br;
  while ((br = read(src_fd, buffer, 512)) > 0) {
    if (const ssize_t bw = write(dst_fd, buffer, br); bw != br) return false;
  }

  if (br == -1) return false;
  LOGN(HELPER, INFO) << "copy " << file << " to " << dest << " successfully."
                     << std::endl;
  return true;
}

bool makeDirectory(const std::string_view path) {
  if (isExists(path)) return false;
  LOGN(HELPER, INFO) << "trying making directory: " << path << std::endl;
  return (mkdir(path.data(), DEFAULT_DIR_PERMS) == 0);
}

bool makeRecursiveDirectory(const std::string_view paths) {
  LOGN(HELPER, INFO) << "make recursive directory requested: " << paths
                     << std::endl;

  char tmp[PATH_MAX];

  snprintf(tmp, sizeof(tmp), "%s", paths.data());
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

  LOGN(HELPER, INFO) << "" << paths << " successfully created." << std::endl;
  return true;
}

bool createFile(const std::string_view path) {
  LOGN(HELPER, INFO) << "create file request: " << path << std::endl;

  if (isExists(path)) return false;

  const int fd = open(path.data(), O_RDONLY | O_CREAT, DEFAULT_FILE_PERMS);
  if (fd == -1) return false;

  close(fd);
  LOGN(HELPER, INFO) << "create file \"" << path << "\" successfull."
                     << std::endl;
  return true;
}

bool createSymlink(const std::string_view entry1,
                   const std::string_view entry2) {
  LOGN(HELPER, INFO) << "symlink \"" << entry1 << "\" to \"" << entry2
                     << "\" requested." << std::endl;
  if (const int ret = symlink(entry1.data(), entry2.data()); ret != 0)
    return false;

  LOGN(HELPER, INFO) << "\"" << entry1 << "\" symlinked to \"" << entry2
                     << "\" successfully." << std::endl;
  return true;
}

bool eraseEntry(const std::string_view entry) {
  LOGN(HELPER, INFO) << "erase \"" << entry << "\" requested." << std::endl;
  if (const int ret = remove(entry.data()); ret != 0) return false;

  LOGN(HELPER, INFO) << "\"" << entry << "\" erased successfully." << std::endl;
  return true;
}

bool eraseDirectoryRecursive(const std::string_view directory) {
  LOGN(HELPER, INFO) << "erase recursive requested: " << directory << std::endl;
  struct stat buf{};
  dirent *entry;

  DIR *dir = opendir(directory.data());
  if (dir == nullptr) return false;

  while ((entry = readdir(dir)) != nullptr) {
    char fullpath[PATH_MAX];

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    snprintf(fullpath, sizeof(fullpath), "%s/%s", directory.data(),
             entry->d_name);

    if (lstat(fullpath, &buf) == -1) {
      closedir(dir);
      return false;
    }

    if (S_ISDIR(buf.st_mode)) {
      if (!eraseDirectoryRecursive(fullpath)) {
        closedir(dir);
        return false;
      }
    } else {
      if (unlink(fullpath) == -1) {
        closedir(dir);
        return false;
      }
    }
  }

  closedir(dir);
  if (rmdir(directory.data()) == -1) return false;

  LOGN(HELPER, INFO) << "\"" << directory << "\" successfully erased."
                     << std::endl;
  return true;
}

std::string readSymlink(const std::string_view entry) {
  LOGN(HELPER, INFO) << "read symlink request: " << entry << std::endl;

  char target[PATH_MAX];
  const ssize_t len = readlink(entry.data(), target, (sizeof(target) - 1));
  if (len == -1) return entry.data();

  target[len] = '\0';
  LOGN(HELPER, INFO) << "\"" << entry << "\" symlink to \"" << target << "\""
                     << std::endl;
  return target;
}

size_t fileSize(const std::string_view file) {
  LOGN(HELPER, INFO) << "get file size request: " << file << std::endl;
  struct stat st{};
  if (stat(file.data(), &st) != 0) return false;
  return static_cast<size_t>(st.st_size);
}
} // namespace Helper
