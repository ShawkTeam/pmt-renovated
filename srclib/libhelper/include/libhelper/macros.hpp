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

#ifndef LIBHELPER_MACROS_HPP
#define LIBHELPER_MACROS_HPP

#include <unistd.h>

constexpr mode_t DEFAULT_FILE_PERMS = 0644;
constexpr mode_t DEFAULT_EXTENDED_FILE_PERMS = 0755;
constexpr mode_t DEFAULT_DIR_PERMS = 0755;
constexpr int YES = 1;
constexpr int NO = 0;

enum sizeCastTypes { B = static_cast<int>('B'), KB = static_cast<int>('K'), MB = static_cast<int>('M'), GB = static_cast<int>('G') };

#define HELPER "libhelper"
#define ERR Helper::Error()

#define B(x) (static_cast<uint64_t(x)> * 8)     // B(4)  = 32 (4 * 8) (bit)
#define KB(x) (static_cast<uint64_t>(x) * 1024) // KB(8) = 8192 (8 * 1024)
#define MB(x) (KB(x) * 1024)                    // MB(4) = 4194304 (KB(4) * 1024)
#define GB(x) (MB(x) * 1024)                    // GB(1) = 1073741824 (MB(1) * 1024)

#define TO_KB(x) (x / 1024)        // TO_KB(1024) = 1
#define TO_MB(x) (TO_KB(x) / 1024) // TO_MB(2048) (2048 / 1024)
#define TO_GB(x) (TO_MB(x) / 1024) // TO_GB(1048576) (TO_MB(1048576) / 1024)

#define STYLE_RESET "\033[0m"
#define BOLD "\033[1m"
#define FAINT "\033[2m"
#define ITALIC "\033[3m"
#define UNDERLINE "\033[4m"
#define BLINC "\033[5m"
#define FAST_BLINC "\033[6m"
#define STRIKE_THROUGHT "\033[9m"
#define NO_UNDERLINE "\033[24m"
#define NO_BLINC "\033[25m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"

#ifndef HELPER_NO_C_TYPE_HANDLERS
// ABORT(message), ex: ABORT("memory error!\n")
#define ABORT(msg)                                                                                                                    \
  do {                                                                                                                                \
    fprintf(stderr, "%s%sCRITICAL ERROR%s: %s\nAborting...\n", BOLD, RED, STYLE_RESET, msg);                                          \
    abort();                                                                                                                          \
  } while (0)

// ERROR(message, exit), ex: ERROR("an error occured.\n", 1)
#define ERROR(msg, code)                                                                                                              \
  do {                                                                                                                                \
    fprintf(stderr, "%s%sERROR%s: %s", BOLD, RED, STYLE_RESET, msg);                                                                  \
    exit(code);                                                                                                                       \
  } while (0)

// WARNING(message), ex: WARNING("using default setting.\n")
#define WARNING(msg) fprintf(stderr, "%s%sWARNING%s: %s", BOLD, YELLOW, STYLE_RESET, msg);

// INFO(message), ex: INFO("operation ended.\n")
#define INFO(msg) fprintf(stdout, "%s%sINFO%s: %s", BOLD, GREEN, STYLE_RESET, msg);
#endif // #ifndef HELPER_NO_C_TYPE_HANDLERS

#define MKVERSION(name)                                                                                                               \
  char vinfo[512];                                                                                                                    \
  sprintf(vinfo,                                                                                                                      \
          "%s %s-%s [%s %s]\nBuildType: %s\nCMakeVersion: %s\nCompilerVersion: "                                                      \
          "%s\nBuildFlags: %s",                                                                                                       \
          name, BUILD_VERSION, COMMIT_ID, BUILD_DATE, BUILD_TIME, BUILD_TYPE, BUILD_CMAKE_VERSION, BUILD_COMPILER_VERSION,            \
          BUILD_FLAGS);                                                                                                               \
  return std::string(vinfo)

#endif // #ifndef LIBHELPER_MACROS_HPP
