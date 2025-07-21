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

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libhelper/lib.hpp>

static FILE* open_file(const std::string_view file, const char* mode)
{
	FILE* fp = fopen(file.data(), mode);
	if (fp == nullptr) {
		throw Helper::Error("Cannot open %s: %s", file.data(), strerror(errno));
		return fp;
	}

	return fp;
}

namespace Helper {

bool writeFile(const std::string_view file, const std::string_view text)
{
	FILE* fp = open_file(file, "a");
	if (fp == nullptr) return false;

	fprintf(fp, "%s", text.data());
	fclose(fp);

	return true;
}

std::optional<std::string> readFile(const std::string_view file)
{
	FILE* fp = open_file(file, "r");
	if (fp == nullptr) return std::nullopt;

	char buffer[1024];
	std::string str;
	while (fgets(buffer, sizeof(buffer), fp)) str += buffer;

	fclose(fp);
	return str;
}

bool copyFile(const std::string_view file, const std::string_view dest)
{
	int src_fd = open(file.data(), O_RDONLY);
	if (src_fd == - 1) {
		throw Error("Cannot open %s: %s", file.data(), strerror(errno));
		return false;
	}

	int dst_fd = open(dest.data(), O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_PERMS);
	if (dst_fd == - 1) {
		throw Error("Cannot create/open %s: %s", dest.data(), strerror(errno));
		return false;
	}

	char buffer[512];
	ssize_t br;

	while ((br = read(src_fd, buffer, 512)) > 0) {
		ssize_t bw = write(dst_fd, buffer, br);
		if (bw != br) {
			throw Error("Cannot write %s: %s", dest.data(), strerror(errno));
			close(src_fd);
			close(dst_fd);
			return false;
		}
	}

	close(src_fd);
	close(dst_fd);
	if (br == -1) {
		throw Error("Cannot read %s: %s", file.data(), strerror(errno));
		return false;
	}

	return true;
}

bool makeDirectory(const std::string_view path)
{
	if (isExists(path)) return false;
	return (mkdir(path.data(), DEFAULT_DIR_PERMS) == 0) ? true : false;
}

bool makeRecursiveDirectory(const std::string_view paths)
{
	char tmp[PATH_MAX], *p;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", paths.data());
	len = strlen(tmp);
	if (tmp[len - 1] == '/') tmp[len - 1] = '\0';

	for (p = tmp + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
			if (access(tmp, F_OK) != 0) {
				if (mkdir(tmp, DEFAULT_DIR_PERMS) != 0 
				    && errno != EEXIST) {
					throw Error("Cannot create directory: %s: %s", tmp, strerror(errno));
					return false;
				}
			}
			*p = '/';
		}
	}

	if (access(tmp, F_OK) != 0) {
		if (mkdir(tmp, DEFAULT_DIR_PERMS) != 0 && errno != EEXIST) {
			throw Error("Cannot create directory: %s: %s", tmp, strerror(errno));
			return false;
		}
	}

	return true;
}

bool createFile(const std::string_view path)
{
	if (isExists(path)) {
		throw Error("%s: is exists", path.data());
		return false;
	}

	int fd = open(path.data(), O_RDONLY | O_CREAT, DEFAULT_FILE_PERMS);
	if (fd == -1) {
		throw Error("Cannot create %s: %s", path.data(), strerror(errno));
		return false;
	}

	close(fd);
	return true;
}

bool createSymlink(const std::string_view entry1, const std::string_view entry2)
{
	int ret = symlink(entry1.data(), entry2.data());
	if (ret != 0)
		throw Error("Cannot symlink %s: %s", entry2.data(), strerror(errno));

	return (ret == 0);
}

bool eraseEntry(const std::string_view entry)
{
	int ret = remove(entry.data());
	if (ret != 0)
		throw Error("Cannot remove %s: %s", entry.data(), strerror(errno));

	return (ret == 0);
}

bool eraseDirectoryRecursive(const std::string_view directory)
{
	struct stat buf;
	struct dirent *entry;

	DIR *dir = opendir(directory.data());
	if (dir == nullptr) {
		throw Error("Cannot open directory %s: %s", directory.data(), strerror(errno));
		return false;
	}

	while ((entry = readdir(dir)) != NULL) {
		char fullpath[PATH_MAX];

		if (strcmp(entry->d_name, ".") == 0 
		    || strcmp(entry->d_name, "..") == 0)
			continue;

		snprintf(fullpath, sizeof(fullpath), "%s/%s", directory.data(), entry->d_name);

		if (lstat(fullpath, &buf) == -1) {
			throw Error("Cannot stat %s: %s", fullpath, strerror(errno));
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
				throw Error("Cannot unlink %s: %s", fullpath, strerror(errno));
				closedir(dir);
				return false;
			}
		}
	}

	closedir(dir);
	if (rmdir(directory.data()) == -1) {
		throw Error("Cannot remove directory %s: %s", directory.data(), strerror(errno));
		return false;
	}

	return true;
}

std::string_view readSymlink(const std::string_view entry)
{
	char target[PATH_MAX];
	ssize_t len = readlink(entry.data(), target, (sizeof(target) - 1));
	if (len == -1) {
		throw Error("Cannot read symlink %s: %s", entry.data(), strerror(errno));
		return entry;
	}

	target[len] = '\0';
	return target;
}

size_t fileSize(const std::string_view file)
{
	struct stat st;
	if (stat(file.data(), &st) != 0) return false;
	return static_cast<size_t>(st.st_size);
}

} // namespace Helper
