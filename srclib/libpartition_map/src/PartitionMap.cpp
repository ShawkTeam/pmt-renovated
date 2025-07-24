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

#include <iostream>
#include <vector>
#include <filesystem>
#include <memory>
#include <vector>
#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <libpartition_map/lib.hpp>
#include <generated/buildInfo.hpp>
#include <string.h>
#include <unistd.h>

static constexpr std::array<std::string_view, 3> defaultEntryList = {
	"/dev/block/by-name",
	"/dev/block/bootdevice/by-name",
	"/dev/block/platform/bootdevice/by-name"
};

namespace PartitionMap {

bool basic_partition_map_builder::_is_real_block_dir(const std::string_view path) const
{
	if (path.find("/block/") == std::string::npos) return false;
	return true;
}

Map_t basic_partition_map_builder::_build_map(std::string_view path, bool logical)
{
	Map_t map;
	std::vector<std::filesystem::directory_entry> entries{std::filesystem::directory_iterator(path), std::filesystem::directory_iterator()};
	std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
		return a.path().filename() < b.path().filename();
	});

	LOGN_IF(MAP, WARNING, entries.empty()) << __func__ << "(): " << path << "is exists but generated vector is empty (std::vector<std::filesystem::directory_entry>)." << std::endl;
	for (const auto& entry : entries) {
		if (entry.path().filename() != "by-uuid"
		    && std::string(entry.path()).find("com.") == std::string::npos)
			map.insert(entry.path().filename().string(), _get_size(entry.path()), logical);
	}

	LOGN(MAP, INFO) << std::boolalpha << __func__ << "(): Map generated successfully. is_logical_map=" << logical << std::endl;
	return map;
}

void basic_partition_map_builder::_insert_logicals(Map_t&& logicals)
{
	_current_map.merge(logicals);
}

void basic_partition_map_builder::_map_build_check() const
{
	if (!_map_builded)
		throw Error("Please build partition map before!");
}

uint64_t basic_partition_map_builder::_get_size(const std::string path)
{
	std::string real = std::filesystem::read_symlink(path);
	int fd = open(real.data(), O_RDONLY);
	if (fd < 0)
		throw Error("Cannot open %s: %s", real.data(), strerror(errno));

	uint64_t size = 0;
	if (ioctl(fd, BLKGETSIZE64, &size) != 0) {
		close(fd);
		throw Error("ioctl() process failed for %s: %s", real.data(), strerror(errno));
	}

	close(fd);
	return size;
}

basic_partition_map_builder::basic_partition_map_builder()
{
	LOGN(MAP, INFO) << __func__ << "(): default constructor called. Starting build." << std::endl;

	for (const auto& path : defaultEntryList) {
		if (std::filesystem::exists(path)) {
			_current_map = _build_map(path);
			if (_current_map.empty()) {
				_any_generating_error = true;
				continue;
			} else {
				_workdir = path;
				break;
			}
		}
	}

	if (_current_map.empty())
		throw Error("Cannot build map by any default search entry.");

	LOGN(MAP, INFO) << __func__ << "(): default constructor successfully ended work." << std::endl;
	_insert_logicals(_build_map("/dev/block/mapper", true));
	_map_builded = true;
}

basic_partition_map_builder::basic_partition_map_builder(const std::string_view path)
{
	LOGN(MAP, INFO) << __func__ << "(): argument-based constructor called. Starting build." << std::endl;

	if (std::filesystem::exists(path)) {
		_is_real_block_dir(path);
		_current_map = _build_map(path);
		if (_current_map.empty()) _any_generating_error = true;
		else _workdir = path;
	} else
		throw Error("Cannot find directory: %s. Cannot build partition map!", path.data());

	LOGN(MAP, INFO) << __func__ << "(): argument-based constructor successfully ended work." << std::endl;
	_insert_logicals(_build_map("/dev/block/mapper", true));
	_map_builded = true;
}

bool basic_partition_map_builder::hasPartition(const std::string_view name) const
{
	_map_build_check();
	return _current_map.find(name);
}

bool basic_partition_map_builder::isLogical(const std::string_view name) const
{
	_map_build_check();
	return _current_map.is_logical(name);
}

void basic_partition_map_builder::clear()
{
	_current_map.clear();
	_workdir.clear();
	_any_generating_error = false;
}

bool basic_partition_map_builder::readDirectory(const std::string_view path)
{
	_map_builded = false;
	LOGN(MAP, INFO) << __func__ << "(): read " << path << " directory request." << std::endl;

	if (std::filesystem::exists(path)) {
		if (!_is_real_block_dir(path)) return false;
		_current_map = _build_map(path);
		if (_current_map.empty()) {
			_any_generating_error = true;
			return false;
		} else _workdir = path;
	} else
		throw Error("Cannot find directory: %s. Cannot build partition map!", path.data());

	LOGN(MAP, INFO) << __func__ << "(): read " << path << " successfull." << std::endl;
	_insert_logicals(_build_map("/dev/block/mapper", true));
	_map_builded = true;
	return true;
}

bool basic_partition_map_builder::empty() const
{
	_map_build_check();
	return _current_map.empty();
}

uint64_t basic_partition_map_builder::sizeOf(const std::string_view name) const
{
	_map_build_check();
	return _current_map.get_size(name);
}

bool operator==(basic_partition_map_builder& lhs, basic_partition_map_builder& rhs)
{
	return lhs._current_map == rhs._current_map;
}

bool operator!=(basic_partition_map_builder& lhs, basic_partition_map_builder& rhs)
{
	return !(lhs == rhs);
}

basic_partition_map_builder::operator bool() const
{
	return !this->_any_generating_error;
}

bool basic_partition_map_builder::operator!() const
{
	return this->_any_generating_error;
}

std::string getLibVersion()
{
	char vinfo[512];
	sprintf(vinfo, "libpartition_map %s [%s %s]\nBuildType: %s\nCMakeVersion: %s\nCompilerVersion: %s\nBuildFlags: %s\n", BUILD_VERSION, BUILD_DATE, BUILD_TIME, BUILD_TYPE, BUILD_CMAKE_VERSION, BUILD_COMPILER_VERSION, BUILD_FLAGS);
	return std::string(vinfo);
}

} // namespace PartitionMap
