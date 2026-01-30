/*
   Copyright 2026 Yağız Zengin

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
#include <dirent.h>
#include <fcntl.h>
#include <iomanip>
#include <libhelper/lib.hpp>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace Helper {
bool writeFile(const std::filesystem::path &file, const std::string_view text) {
  LOGN(HELPER, INFO) << "write " << std::quoted(text) << " to " << std::quoted(file.c_str()) << "requested." << std::endl;
  garbageCollector collector;

  FILE *fp = openAndAddToCloseList(file, collector, "a");
  if (fp == nullptr) return false;
  fprintf(fp, "%s", text.data());

  LOGN(HELPER, INFO) << "write " << std::quoted(file.string()) << " successfully." << std::endl;
  return true;
}

std::optional<std::string> readFile(const std::filesystem::path &file) {
  LOGN(HELPER, INFO) << "read " << std::quoted(file.string()) << " requested." << std::endl;
  garbageCollector collector;

  FILE *fp = openAndAddToCloseList(file, collector, "r");
  if (fp == nullptr) return std::nullopt;

  char buffer[1024];
  std::string str;
  while (fgets(buffer, sizeof(buffer), fp))
    str += buffer;

  LOGN(HELPER, INFO) << "read " << file << " successfully, read text: " << std::quoted(str) << std::endl;
  return str;
}

bool copyFile(const std::filesystem::path &file, const std::filesystem::path &dest) {
  LOGN(HELPER, INFO) << "copy " << std::quoted(file.string()) << " to " << std::quoted(dest.string()) << " requested." << std::endl;
  garbageCollector collector;

  const int src_fd = openAndAddToCloseList(file, collector, O_RDONLY);
  if (src_fd == -1) return false;

  const int dst_fd = openAndAddToCloseList(dest, collector, O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_PERMS);
  if (dst_fd == -1) return false;

  char buffer[512];
  ssize_t br;
  while ((br = read(src_fd, buffer, 512)) > 0) {
    if (const ssize_t bw = write(dst_fd, buffer, br); bw != br) return false;
  }

  if (br == -1) return false;
  LOGN(HELPER, INFO) << "copy " << std::quoted(file.string()) << " to " << std::quoted(dest.string()) << " successfully." << std::endl;
  return true;
}

bool makeDirectory(const std::filesystem::path &path) {
  if (isExists(path)) return false;
  LOGN(HELPER, INFO) << "trying making directory: " << std::quoted(path.string()) << std::endl;
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

  const int fd = open(path.c_str(), O_RDONLY | O_CREAT, DEFAULT_FILE_PERMS);
  if (fd == -1) return false;

  close(fd);
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
  garbageCollector collector;

  DIR *dir = openAndAddToCloseList(directory, collector);
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

int64_t fileSize(const std::filesystem::path &file) {
  LOGN(HELPER, INFO) << "get file size request: " << std::quoted(file.string()) << std::endl;
  struct stat st{};
  if (stat(file.c_str(), &st) != 0) return -1;
  return st.st_size;
}
} // namespace Helper
