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

#ifndef LIBHELPER_LIB_HPP
#define LIBHELPER_LIB_HPP

#define LIBHELPER_MAJOR 1
#define LIBHELPER_MINOR 0
#define LIBHELPER_PATCH 0

#include <string>
#include <string_view>
#include <sstream>
#include <exception>
#include <optional>

#ifndef ONLY_HELPER_MACROS

enum LogLevels {
	INFO = (int)'I',
	WARNING = (int)'W',
	ERROR = (int)'E',
	ABORT = (int)'A'
};

constexpr mode_t DEFAULT_FILE_PERMS = 0644;
constexpr mode_t DEFAULT_DIR_PERMS  = 0755;
constexpr int    YES = 1;
constexpr int    NO  = 0;

namespace Helper {

// Logging
class Logger {
private:
	LogLevels _level;
	std::ostringstream _oss;
	const char *_logFile, *_program_name, *_file;
	int _line;

public:
	Logger(LogLevels level, const char* file, const char* name, const char* sfile, int line);
	~Logger();

	template <typename T>
	Logger& operator<<(const T& msg)
	{
		_oss << msg;
		return *this;
	}
	Logger& operator<<(std::ostream& (*msg)(std::ostream&));
};

class LoggingProperties {
public:
	static std::string_view FILE, NAME;
	static bool PRINT;

	static void set(std::string_view name, std::string_view file);
	static void setProgramName(std::string_view name);
	static void setLogFile(std::string_view file);
	static void setPrinting(int state);
	static void reset();
};

// Throwable error class
class Error : public std::exception {
private:
	std::string _message;

public:
	Error(const char* format, ...);

	const char* what() const noexcept override;
};

// Checkers
bool hasSuperUser();
bool isExists(const std::string_view entry);
bool fileIsExists(const std::string_view file);
bool directoryIsExists(const std::string_view directory);
bool linkIsExists(const std::string_view entry);
bool isLink(const std::string_view entry);
bool isSymbolicLink(const std::string_view entry);
bool isHardLink(const std::string_view entry);
bool areLinked(const std::string_view entry1, const std::string_view entry2);

// File I/O
bool writeFile(const std::string_view file, const std::string_view text);
std::optional<std::string> readFile(const std::string_view file);

// Creators
bool makeDirectory(const std::string_view path);
bool makeRecursiveDirectory(const std::string_view paths);
bool createFile(const std::string_view path);
bool createSymlink(const std::string_view entry1, const std::string_view entry2);

// Removers
bool eraseEntry(const std::string_view entry);
bool eraseDirectoryRecursive(const std::string_view directory);

// Getters
size_t fileSize(const std::string_view file);
std::string_view readSymlink(const std::string_view entry);

// SHA-256
bool sha256Compare(const std::string_view file1, const std::string_view file2);
std::optional<std::string_view> sha256Of(const std::string_view path);

// Utilities
bool copyFile(const std::string_view file, const std::string_view dest);
bool runCommand(const std::string_view cmd);
bool confirmPropt(const std::string_view message);
std::string currentWorkingDirectory();
std::string currentDate();
std::string currentTime();
std::string runCommandWithOutput(const std::string_view cmd);
std::string pathJoin(std::string base, std::string relative);
std::string pathBasename(const std::string_view entry);
std::string pathDirname(const std::string_view entry);

// Library-specif
std::string getLibVersion();

} // namespace Helper

#endif // #ifndef ONLY_HELPER_MACROS

#define KB(x) (x * 1024)     // KB(8) = 8192 (8 * 1024)
#define MB(x) (KB(x) * 1024) // MB(4) = 4194304 (KB(4) * 1024)
#define GB(x) (MB(x) * 1024) // GB(1) = 1073741824 (MB(1) * 1024)

#define STYLE_RESET		"\033[0m"
#define BOLD			"\033[1m"
#define FAINT			"\033[2m"
#define ITALIC			"\033[3m"
#define UNDERLINE		"\033[4m"
#define BLINC			"\033[5m"
#define FAST_BLINC		"\033[6m"
#define STRIKE_THROUGHT	"\033[9m"
#define NO_UNDERLINE	"\033[24m"
#define NO_BLINC		"\033[25m"
#define RED				"\033[31m"
#define GREEN			"\033[32m"
#define YELLOW			"\033[33m"

#ifndef NO_C_TYPE_HANDLERS
// ABORT(message), ex: ABORT("memory error!\n")
#define ABORT(msg) \
	do { \
		fprintf(stderr, "%s%sCRITICAL ERROR%s: %s\nAborting...\n", BOLD, RED, STYLE_RESET, msg); \
		abort(); \
	} while (0)

// ERROR(message, exit), ex: ERROR("an error occured.\n", 1)
#define ERROR(msg, code) \
	do { \
		fprintf(stderr, "%s%sERROR%s: %s", BOLD, RED, STYLE_RESET, msg); \
		exit(code); \
	} while (0)

// WARNING(message), ex: WARNING("using default setting.\n")
#define WARNING(msg) \
	fprintf(stderr, "%s%sWARNING%s: %s", BOLD, YELLOW, STYLE_RESET, msg);

// INFO(message), ex: INFO("operation ended.\n")
#define INFO(msg) \
	fprintf(stdout, "%s%sINFO%s: %s", BOLD, GREEN, STYLE_RESET, msg);
#endif // #ifndef NO_C_TYPE_HANDLERS

#define LOG(level) Helper::Logger(level, Helper::LoggingProperties::FILE.data(), Helper::LoggingProperties::NAME.data(), __FILE__, __LINE__)

#endif // #ifndef LIBHELPER_LIB_HPP
