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
#include <memory>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <libgen.h>
#include <libhelper/lib.hpp>
#include <generated/buildInfo.hpp>

namespace Helper {

bool runCommand(const std::string_view cmd)
{
	return (system(cmd.data()) == 0) ? true : false;
}

bool confirmPropt(const std::string_view message)
{
	char p;

	printf("%s [ y / n ]: ", message.data());
	std::cin >> p;

	if (p == 'y' || p == 'Y') return true;
	else if (p == 'n' || p == 'N') return false;
	else {
		printf("Unexpected answer: '%c'. Try again.\n", p);
		return confirmPropt(message);
	}

	return false;
}

std::string currentWorkingDirectory()
{
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) == nullptr) return std::string();
	return cwd;
}

std::string currentDate()
{
	time_t t = time(nullptr);
	struct tm *date = localtime(&t);

	if (date)
		return std::string(
			   std::to_string(date->tm_mday) + "/" +
			   std::to_string(date->tm_mon + 1) + "/" +
			   std::to_string(date->tm_year + 1900));
	return "--/--/----";
}

std::string currentTime()
{
	time_t t = time(nullptr);
	struct tm *date = localtime(&t);

	if (date)
		return std::string(
			   std::to_string(date->tm_hour) + ":" +
			   std::to_string(date->tm_min) + ":" +
			   std::to_string(date->tm_sec));
	return "--:--:--";
}

std::string runCommandWithOutput(const std::string_view cmd)
{
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.data(), "r"), pclose);
	if (!pipe) {
		throw Error("Cannot run command: %s", cmd.data());
		return {};
	}

	std::string end;
	char buffer[1024];

	while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) end += buffer;

	return end;
}

std::string pathJoin(std::string base, std::string relative)
{
	if (base.back() != '/') base += '/';
	if (relative[0] == '/') relative.erase(0, 1);
	base += relative;
	return base;
}

std::string pathBasename(const std::string_view entry)
{
	if (!isExists(entry)) {
		throw Error("No such file or directory: %s", entry.data());
		return {};
	}

	char* base = basename((char*)entry.data());
	return (base == nullptr) ? std::string() : std::string(base);
}

std::string pathDirname(const std::string_view entry)
{
	if (!isExists(entry)) {
		throw Error("No such file or directory: %s", entry.data());
		return {};
	}

	char* base = dirname((char*)entry.data());
	return (base == nullptr) ? std::string() : std::string(base);
}

std::string getLibVersion()
{
	char vinfo[512];
	sprintf(vinfo, "libhelper %s [%s %s]\nBuildType: %s\nCMakeVersion: %s\nCompilerVersion: %s\nBuildFlags: %s\n", BUILD_VERSION, BUILD_DATE, BUILD_TIME, BUILD_TYPE, BUILD_CMAKE_VERSION, BUILD_COMPILER_VERSION, BUILD_FLAGS);
	return std::string(vinfo);
}

} // namespace Helper
