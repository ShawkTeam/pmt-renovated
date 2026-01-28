/*
   Copyright 2026 Yağız Zengin

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

#ifndef LIBPARTITION_MAP_REDEFINE_LOGGING_MACROS_HPP
#define LIBPARTITION_MAP_REDEFINE_LOGGING_MACROS_HPP

#include <libhelper/lib.hpp>

#define MAP "libpartition_map"

#undef LOG
#define LOG(level) Helper::Logger(level, __func__, Helper::LoggingProperties::FILE.data(), MAP, __FILE__, __LINE__)
#define LOGI Helper::Logger(LogLevels::INFO, __func__, Helper::LoggingProperties::FILE.data(), MAP, __FILE__, __LINE__)
#define LOGW                                                                                                           \
  Helper::Logger(LogLevels::WARNING, __func__, Helper::LoggingProperties::FILE.data(), MAP, __FILE__, __LINE__)
#define LOGE Helper::Logger(LogLevels::ERROR, __func__, Helper::LoggingProperties::FILE.data(), MAP, __FILE__, __LINE__)
#define LOGA Helper::Logger(LogLevels::ABORT, __func__, Helper::LoggingProperties::FILE.data(), MAP, __FILE__, __LINE__)

#endif // LIBPARTITION_MAP_REDEFINE_LOGGING_MACROS_HPP
