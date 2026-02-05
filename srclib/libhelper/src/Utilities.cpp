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

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstdarg>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <libhelper/lib.hpp>
#ifndef ANDROID_BUILD
#include <sys/_system_properties.h>
#include <generated/buildInfo.hpp>
#else
#include <sys/system_properties.h>
#endif
#include <cutils/android_reboot.h>

#ifdef __ANDROID__
// From system/core/libcutils/android_reboot.cpp android16-s2-release
int android_reboot(const unsigned cmd, int /*flags*/, const char *arg) {
  int ret;
  const char *restart_cmd = nullptr;
  char *prop_value;

  switch (cmd) {
    case ANDROID_RB_RESTART: // deprecated
    case ANDROID_RB_RESTART2:
      restart_cmd = "reboot";
      break;
    case ANDROID_RB_POWEROFF:
      restart_cmd = "shutdown";
      break;
    case ANDROID_RB_THERMOFF:
      restart_cmd = "shutdown,thermal";
      break;
    default:;
  }

  if (!restart_cmd) return -1;
  if (arg && arg[0])
    ret = asprintf(&prop_value, "%s,%s", restart_cmd, arg);
  else
    ret = asprintf(&prop_value, "%s", restart_cmd);

  if (ret < 0) return -1;
  ret = __system_property_set(ANDROID_RB_PROPERTY, prop_value);
  free(prop_value);
  return ret;
}
#endif

namespace Helper {
namespace LoggingProperties {
std::string_view FILE = "last_logs.log", NAME = "main";
bool PRINT = NO, DISABLE = NO;

void reset() {
  FILE = "last_logs.log";
  NAME = "main";
  PRINT = NO;
}

void set(const std::filesystem::path &file, std::string_view name) {
  if (!file.string().empty()) FILE = file.c_str();
  if (name.data() != nullptr) NAME = name;
}

void setProgramName(const std::string_view name) { NAME = name; }
void setLogFile(std::string_view file) { FILE = file; }
} // namespace LoggingProperties

bool runCommand(const std::string_view cmd) {
  LOGN(HELPER, INFO) << "run command request: " << cmd << std::endl;
  return (system(cmd.data()) == 0) ? true : false;
}

bool confirmPropt(const std::string_view message) {
  LOGN(HELPER, INFO) << "create confirm propt request. Creating." << std::endl;
  char p;

  printf("%s [ y / n ]: ", message.data());
  std::cin >> p;

  if (p == 'y' || p == 'Y') return true;
  if (p == 'n' || p == 'N') return false;

  printf("Unexpected answer: '%c'. Try again.\n", p);
  return confirmPropt(message);
}

std::string currentWorkingDirectory() {
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) == nullptr) return {};
  return cwd;
}

std::string currentDate() {
  const time_t t = time(nullptr);

  if (const tm *date = localtime(&t))
    return std::string(std::to_string(date->tm_mday) + "/" + std::to_string(date->tm_mon + 1) + "/" +
                       std::to_string(date->tm_year + 1900));
  return {};
}

std::string currentTime() {
  const time_t t = time(nullptr);

  if (const tm *date = localtime(&t))
    return std::string(std::to_string(date->tm_hour) + ":" + std::to_string(date->tm_min) + ":" + std::to_string(date->tm_sec));
  return {};
}

std::pair<std::string, int> runCommandWithOutput(const std::string_view cmd) {
  LOGN(HELPER, INFO) << "run command and catch out request: " << cmd << std::endl;

  FILE *pipe = popen(cmd.data(), "r");
  if (!pipe) return {};

  std::unique_ptr<FILE, decltype(&pclose)> pipe_holder(pipe, pclose);

  std::string output;
  char buffer[1024];

  while (fgets(buffer, sizeof(buffer), pipe_holder.get()) != nullptr)
    output += buffer;

  FILE *raw = pipe_holder.release();
  const int status = pclose(raw);
  return {output, (WIFEXITED(status) ? WEXITSTATUS(status) : -1)};
}

std::filesystem::path pathJoin(std::filesystem::path base, const std::filesystem::path &relative) {
  base /= relative;
  return base;
}

std::filesystem::path pathBasename(const std::filesystem::path &entry) { return entry.filename(); }

std::filesystem::path pathDirname(const std::filesystem::path &entry) { return entry.parent_path(); }

bool changeMode(const std::filesystem::path &file, const mode_t mode) {
  LOGN(HELPER, INFO) << "change mode request: " << file << ". As mode: " << mode << std::endl;
  return chmod(file.c_str(), mode) == 0;
}

bool changeOwner(const std::filesystem::path &file, const uid_t uid, const gid_t gid) {
  LOGN(HELPER, INFO) << "change owner request: " << file << ". As owner:group: " << uid << ":" << gid << std::endl;
  return chown(file.c_str(), uid, gid) == 0;
}

int openAndAddToCloseList(const std::filesystem::path &path, garbageCollector &collector, const int flags, const mode_t mode) {
  const int fd = mode == 0 ? open(path.c_str(), flags) : open(path.c_str(), flags, mode);
  collector.closeAfterProgress(fd);
  return fd;
}

FILE *openAndAddToCloseList(const std::filesystem::path &path, garbageCollector &collector, const char *mode) {
  FILE *fp = fopen(path.c_str(), mode);
  collector.closeAfterProgress(fp);
  return fp;
}

DIR *openAndAddToCloseList(const std::filesystem::path &path, garbageCollector &collector) {
  DIR *dp = opendir(path.c_str());
  collector.closeAfterProgress(dp);
  return dp;
}

#ifdef __ANDROID__
std::string getProperty(const std::string_view prop) {
  char val[PROP_VALUE_MAX];
  const int x = __system_property_get(prop.data(), val);
  return x > 0 ? val : "ERROR";
}

bool androidReboot(const std::string_view arg) {
  LOGN(HELPER, INFO) << "reboot request sent!!!" << std::endl;

  unsigned cmd = ANDROID_RB_RESTART2;
  if (const std::string prop = getProperty("ro.build.version.sdk"); prop != "ERROR") {
    if (std::stoi(prop) < 26) cmd = ANDROID_RB_RESTART;
  }

  return android_reboot(cmd, 0, arg.empty() ? nullptr : arg.data()) != -1;
}
#endif

uint64_t getRandomOffset(const uint64_t size, const uint64_t bufferSize) {
  if (size <= bufferSize) return 0;
  const uint64_t maxOffset = size - bufferSize;
  static std::mt19937_64 generator(std::random_device{}());
  std::uniform_int_distribution<uint64_t> distribution(0, maxOffset - 1);
  return distribution(generator);
}

int convertTo(const uint64_t size, const sizeCastTypes type) {
  if (type == KB) return TO_KB(size);
  if (type == MB) return TO_MB(size);
  if (type == GB) return TO_GB(size);
  return static_cast<int>(size);
}

std::string multipleToString(const sizeCastTypes type) {
  if (type == KB) return "KB";
  if (type == MB) return "MB";
  if (type == GB) return "GB";
  return "B";
}

std::string format(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char str[1024];
  vsnprintf(str, sizeof(str), format, args);
  va_end(args);
  return str;
}

std::string getLibVersion() { MKVERSION("libhelper"); }
} // namespace Helper
