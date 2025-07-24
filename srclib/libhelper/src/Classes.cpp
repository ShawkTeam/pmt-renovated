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

#include <exception>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <stdarg.h>
#include <libhelper/lib.hpp>

namespace Helper {

Error::Error(const char* format, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	_message = std::string(buf);
	LOGN(HELPER, ERROR) << "Error::Error(): " << _message << std::endl;
}

const char* Error::what() const noexcept
{
	return _message.data();
}

Logger::Logger(LogLevels level, const char* file, const char* name, const char* sfile, int line) : _level(level), _logFile(file), _program_name(name), _file(sfile), _line(line) {}

Logger::~Logger()
{
	if (LoggingProperties::DISABLE) return;
	char str[1024];
	snprintf(str, sizeof(str), "<%c> [ <prog %s> <on %s:%d> %s %s] %s",
	       (char)_level,
	       _program_name,
	       basename((char*)_file),
	       _line,
	       currentDate().data(),
	       currentTime().data(),
	       _oss.str().data());

	if (!isExists(_logFile)) createFile(_logFile);

	FILE* fp = fopen(_logFile, "a");
	if (fp != NULL) {
		fprintf(fp, "%s", str);
		fclose(fp);
	}

	if (LoggingProperties::PRINT) printf("%s\n", str);
}

Logger& Logger::operator<<(std::ostream& (*msg)(std::ostream&))
{
	_oss << msg;
	return *this;
}

} // namespace Helper
