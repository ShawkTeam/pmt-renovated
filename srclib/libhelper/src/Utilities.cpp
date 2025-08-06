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
#include <string_view>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <ctime>
#include <libgen.h>
#include <libhelper/lib.hpp>
#include <generated/buildInfo.hpp>
#include <sys/stat.h>

namespace Helper {
namespace LoggingProperties {

std::string_view FILE = "last_logs.log", NAME = "main";
bool PRINT = NO, DISABLE = NO;

void reset()
{
	FILE = "last_logs.log";
	NAME = "main";
	PRINT = NO;
}

void set(std::string_view file, std::string_view name)
{
	if (file.data() != nullptr) FILE = file;
	if (name.data() != nullptr) NAME = name;
}

void setProgramName(std::string_view name) { NAME = name; }
void setLogFile(std::string_view file) { FILE = file; }

void setPrinting(int state)
{
	if (state == 1 || state == 0) PRINT = state;
	else PRINT = NO;
}

void setLoggingState(int state)
{
	if (state == 1 || state == 0) DISABLE = state;
	else DISABLE = NO;
}

} // namespace LoggingProperties

bool runCommand(const std::string_view cmd)
{
	LOGN(HELPER, INFO) << "run command request: " << cmd << std::endl;
	return (system(cmd.data()) == 0) ? true : false;
}

bool confirmPropt(const std::string_view message)
{
	LOGN(HELPER, INFO) << "create confirm propt request. Creating." << std::endl;
	char p;

	printf("%s [ y / n ]: ", message.data());
	std::cin >> p;

	if (p == 'y' || p == 'Y') return true;
	if (p == 'n' || p == 'N') return false;

	printf("Unexpected answer: '%c'. Try again.\n", p);
	return confirmPropt(message);
}

std::string currentWorkingDirectory()
{
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) == nullptr) return {};
	return cwd;
}

std::string currentDate()
{
	const time_t t = time(nullptr);

	if (const tm *date = localtime(&t))
		return std::string(
			   std::to_string(date->tm_mday) + "/" +
			   std::to_string(date->tm_mon + 1) + "/" +
			   std::to_string(date->tm_year + 1900));
	return "--/--/----";
}

std::string currentTime()
{
	const time_t t = time(nullptr);

	if (const tm *date = localtime(&t))
		return std::string(
			   std::to_string(date->tm_hour) + ":" +
			   std::to_string(date->tm_min) + ":" +
			   std::to_string(date->tm_sec));
	return "--:--:--";
}

std::string runCommandWithOutput(const std::string_view cmd)
{
	LOGN(HELPER, INFO) << "run command and catch out request: " << cmd << std::endl;

	FILE* pipe = popen(cmd.data(), "r");
	if (!pipe) return {};

	std::unique_ptr<FILE, decltype(&pclose)> pipe_holder(pipe, pclose);

	std::string output;
	char buffer[1024];

	while (fgets(buffer, sizeof(buffer), pipe_holder.get()) != nullptr) output += buffer;

	return output;
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
	char* base = basename(const_cast<char *>(entry.data()));
	return (base == nullptr) ? std::string() : std::string(base);
}

std::string pathDirname(const std::string_view entry)
{
	char* base = dirname(const_cast<char *>(entry.data()));
	return (base == nullptr) ? std::string() : std::string(base);
}

bool changeMode(const std::string_view file, const mode_t mode)
{
	LOGN(HELPER, INFO) << "change mode request: " << file << ". As mode: " << mode << std::endl;
	return chmod(file.data(), mode) == 0;
}

bool changeOwner(const std::string_view file, const uid_t uid, const gid_t gid)
{
	LOGN(HELPER, INFO) << "change owner request: " << file << ". As owner:group: " << uid << ":" << gid << std::endl;
	return chown(file.data(), uid, gid) == 0;
}

std::string getLibVersion()
{
	MKVERSION("libhelper");
}

} // namespace Helper
