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

#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <libhelper/lib.hpp>
#include <picosha2.h>

namespace Helper {
std::optional<std::string> sha256Of(const std::filesystem::path &path) {
  LOGN(HELPER, INFO) << "get sha256 of " << std::quoted(path.string()) << " request. Getting full path (if input is link and exists)."
                     << std::endl;
  std::string fp = (isLink(path)) ? readSymlink(path) : std::string(path);

  if (!fileIsExists(fp)) throw ERR << "Is not exists or not file: " << fp;

  if (const std::ifstream file(fp, std::ios::binary); !file) throw ERR << "Cannot open file: " << fp;

  std::vector<unsigned char> hash(picosha2::k_digest_size);
  picosha2::hash256(fp, hash.begin(), hash.end());
  LOGN(HELPER, INFO) << "get sha256 of " << std::quoted(path.string()) << " successfully." << std::endl;
  return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
}

bool sha256Compare(const std::filesystem::path &file1, const std::filesystem::path &file2) {
  LOGN(HELPER, INFO) << "comparing sha256 signatures of input files." << std::endl;
  const auto f1 = sha256Of(file1);
  const auto f2 = sha256Of(file2);
  if (f1->empty() || f2->empty()) return false;
  LOGN_IF(HELPER, INFO, *f1 == *f2) << "(): input files is contains same sha256 signature." << std::endl;
  return *f1 == *f2;
}

} // namespace Helper
