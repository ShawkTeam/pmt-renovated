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

#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <optional>
#include <picosha2.h>
#include <sys/stat.h>
#include <libhelper/lib.hpp>

namespace Helper {

std::optional<std::string_view> sha256Of(const std::string_view path)
{
	if (!fileIsExists(path)) {
		throw Error("Is not exists or not file: %s", path.data());
		return std::nullopt;
	}

	std::ifstream file(path, std::ios::binary);
	if (!file) {
		throw Error("Cannot open file: %s", path.data());
		return std::nullopt;
	}

	std::vector<unsigned char> hash(picosha2::k_digest_size);
	picosha2::hash256(path, hash.begin(), hash.end());
	return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
}

bool sha256Compare(const std::string_view file1, const std::string_view file2)
{
	auto f1 = sha256Of(file1);
	auto f2 = sha256Of(file2);
	if (f1->empty() || f2->empty()) return false;
	return (*f1 == *f2);
}

} // namespace Helper
