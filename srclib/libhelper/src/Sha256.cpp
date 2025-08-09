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
	std::optional<std::string> sha256Of(const std::string_view path) {
		LOGN(HELPER, INFO) << "get sha256 of \"" << path <<
				"\" request. Getting full path (if input is link and exists)." << std::endl;
		std::string fp = (isLink(path)) ? readSymlink(path) : std::string(path);

		if (!fileIsExists(fp)) throw Error("Is not exists or not file: %s", fp.data());

		if (const std::ifstream file(fp, std::ios::binary); !file) throw Error("Cannot open file: %s", fp.data());

		std::vector<unsigned char> hash(picosha2::k_digest_size);
		picosha2::hash256(fp, hash.begin(), hash.end());
		LOGN(HELPER, INFO) << "get sha256 of \"" << path << "\" successfully." << std::endl;
		return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
	}

	bool sha256Compare(const std::string_view file1, const std::string_view file2) {
		LOGN(HELPER, INFO) << "comparing sha256 signatures of input files." << std::endl;
		const auto f1 = sha256Of(file1);
		const auto f2 = sha256Of(file2);
		if (f1->empty() || f2->empty()) return false;
		LOGN_IF(HELPER, INFO, *f1 == *f2) << "(): input files is contains same sha256 signature." << std::endl;
		return (*f1 == *f2);
	}
} // namespace Helper
