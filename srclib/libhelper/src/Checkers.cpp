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

#include <__filesystem/path.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libhelper/functions.hpp>
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

  return st1 == st2;
}
} // namespace Helper
