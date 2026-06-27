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
#include <optional>
#include <string>
#include <vector>
#include <libhelper/functions.hpp>
#include <openssl/sha.h>

namespace Helper {
static std::string bytesToHexString(const unsigned char *bytes, size_t length) {
  std::ostringstream oss;
  for (size_t i = 0; i < length; ++i)
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
  return oss.str();
}

std::optional<std::string> sha256Of(const std::filesystem::path &path) {
  Log::info("Trying to get sha256 of {}.", std::quoted_string(path));

  const std::string fp = (isLink(path)) ? readSymlink(path) : path.string();
  if (!isExists(fp)) throw Error("Is not exists or not file: {}", fp);

  std::ifstream file(fp, std::ios::binary);
  if (!file) throw Error("Cannot open file: {}", fp);

  SHA256_CTX sha256Context;
  if (!SHA256_Init(&sha256Context)) throw Error("SHA256_Init failed for: {}", fp);

  constexpr size_t bufferSize = 4096;
  std::vector<char> buffer(bufferSize);

  while (file.read(buffer.data(), bufferSize) || file.gcount() > 0) {
    if (!SHA256_Update(&sha256Context, buffer.data(), file.gcount())) throw Error("SHA256_Update failed during reading: {}", fp);
  }

  unsigned char hash[SHA256_DIGEST_LENGTH];
  if (!SHA256_Final(hash, &sha256Context)) {
    throw Error("SHA256_Final failed for: {}", fp);
  }

  Log::info("Readed sha256 of {}", std::quoted_string(path));
  return bytesToHexString(hash, SHA256_DIGEST_LENGTH);
}

bool sha256Compare(const std::filesystem::path &file1, const std::filesystem::path &file2) {
  Log::info("Comparing sha256 values of input files.");
  const auto f1 = sha256Of(file1);
  const auto f2 = sha256Of(file2);
  if (!f1 || !f2 || f1->empty() || f2->empty()) return false;
  Log::info("Input files are {}.", *f1 == *f2 ? "same" : "not same");
  return *f1 == *f2;
}

} // namespace Helper
