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

#include <__filesystem/path.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libhelper/lib.hpp>
#include <private/android_filesystem_config.h>

namespace Helper {
bool hasSuperUser() { return getuid() == AID_ROOT; }
bool hasAdbPermissions() { return getuid() == AID_SHELL; }

bool isExists(const std::filesystem::path &entry) {
  struct stat st{};
  return stat(entry.c_str(), &st) == 0;
}

bool fileIsExists(const std::filesystem::path &file) {
  struct stat st{};
  if (stat(file.c_str(), &st) != 0) return false;
  return S_ISREG(st.st_mode);
}

bool directoryIsExists(const std::filesystem::path &directory) {
  struct stat st{};
  if (stat(directory.c_str(), &st) != 0) return false;
  return S_ISDIR(st.st_mode);
}

bool linkIsExists(const std::filesystem::path &entry) { return isLink(entry) || isHardLink(entry); }

bool isLink(const std::filesystem::path &entry) {
  struct stat st{};
  if (lstat(entry.c_str(), &st) != 0) return false;
  return S_ISLNK(st.st_mode);
}

bool isSymbolicLink(const std::filesystem::path &entry) { return isLink(entry); }

bool isHardLink(const std::filesystem::path &entry) {
  struct stat st{};
  if (lstat(entry.c_str(), &st) != 0) return false;
  return st.st_nlink >= 2;
}

bool areLinked(const std::filesystem::path &entry1, const std::filesystem::path &entry2) {
  const std::string st1 = isSymbolicLink(entry1) ? readSymlink(entry1) : entry1.string();
  const std::string st2 = isSymbolicLink(entry2) ? readSymlink(entry2) : entry2.string();

  return (st1 == st2);
}
} // namespace Helper
