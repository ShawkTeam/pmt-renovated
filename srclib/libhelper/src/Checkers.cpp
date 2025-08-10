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

#include <cstdio>
#include <cstdlib>
#include <libhelper/lib.hpp>
#include <private/android_filesystem_config.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Helper {
bool hasSuperUser() { return (getuid() == AID_ROOT); }
bool hasAdbPermissions() { return (getuid() == AID_ADB); }

bool isExists(const std::string_view entry) {
  struct stat st{};
  return (stat(entry.data(), &st) == 0);
}

bool fileIsExists(const std::string_view file) {
  struct stat st{};
  if (stat(file.data(), &st) != 0) return false;
  return S_ISREG(st.st_mode);
}

bool directoryIsExists(const std::string_view directory) {
  struct stat st{};
  if (stat(directory.data(), &st) != 0) return false;
  return S_ISDIR(st.st_mode);
}

bool linkIsExists(const std::string_view entry) {
  return (isLink(entry) || isHardLink(entry));
}

bool isLink(const std::string_view entry) {
  struct stat st{};
  if (lstat(entry.data(), &st) != 0) return false;
  return S_ISLNK(st.st_mode);
}

bool isSymbolicLink(const std::string_view entry) { return isLink(entry); }

bool isHardLink(const std::string_view entry) {
  struct stat st{};
  if (lstat(entry.data(), &st) != 0) return false;
  return (st.st_nlink >= 2);
}

bool areLinked(const std::string_view entry1, const std::string_view entry2) {
  const std::string st1 = (isSymbolicLink(entry1)) ? readSymlink(entry1)
                                                   : std::string(entry1.data());
  const std::string st2 = (isSymbolicLink(entry2)) ? readSymlink(entry2)
                                                   : std::string(entry2.data());

  return (st1 == st2);
}
} // namespace Helper
