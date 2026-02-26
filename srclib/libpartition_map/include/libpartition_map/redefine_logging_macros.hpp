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

#ifndef LIBPARTITION_MAP_REDEFINE_LOGGING_MACROS_HPP
#define LIBPARTITION_MAP_REDEFINE_LOGGING_MACROS_HPP

#include <libhelper/logging.hpp>

#define MAP "libpartition_map"

#undef LOG
#define LOG(level) Helper::Logger(level, __func__, Helper::Logger::Properties::FILE.c_str(), MAP, __FILE__, __LINE__)
#define LOGI Helper::Logger(Helper::LogLevels::INFO, __func__, Helper::Logger::Properties::FILE.c_str(), MAP, __FILE__, __LINE__)
#define LOGW Helper::Logger(Helper::LogLevels::WARNING, __func__, Helper::Logger::Properties::FILE.c_str(), MAP, __FILE__, __LINE__)
#define LOGE Helper::Logger(Helper::LogLevels::ERROR, __func__, Helper::Logger::Properties::FILE.c_str(), MAP, __FILE__, __LINE__)
#define LOGA Helper::Logger(Helper::LogLevels::ABORT, __func__, Helper::Logger::Properties::FILE.c_str(), MAP, __FILE__, __LINE__)

#endif // LIBPARTITION_MAP_REDEFINE_LOGGING_MACROS_HPP
