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

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cutils/android_reboot.h>
#include <fcntl.h>
#ifndef ANDROID_BUILD
#include <generated/buildInfo.hpp>
#include <sys/_system_properties.h>
#else
#include <sys/system_properties.h>
#endif
#include <iostream>
#include <libgen.h>
#include <libhelper/lib.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <sys/_system_properties.h>
#include <cutils/android_reboot.h>
#include <sys/stat.h>
#include <unistd.h>

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
  }

  if (!restart_cmd) return -1;
  if (arg && arg[0]) ret = asprintf(&prop_value, "%s,%s", restart_cmd, arg);
  else ret = asprintf(&prop_value, "%s", restart_cmd);

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

void set(std::string_view file, std::string_view name) {
  if (file.data() != nullptr) FILE = file;
  if (name.data() != nullptr) NAME = name;
}

void setProgramName(const std::string_view name) { NAME = name; }
void setLogFile(const std::string_view file) { FILE = file; }
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
    return std::string(std::to_string(date->tm_mday) + "/" +
                       std::to_string(date->tm_mon + 1) + "/" +
                       std::to_string(date->tm_year + 1900));
  return {};
}

std::string currentTime() {
  const time_t t = time(nullptr);

  if (const tm *date = localtime(&t))
    return std::string(std::to_string(date->tm_hour) + ":" +
                       std::to_string(date->tm_min) + ":" +
                       std::to_string(date->tm_sec));
  return {};
}

std::pair<std::string, int> runCommandWithOutput(const std::string_view cmd) {
  LOGN(HELPER, INFO) << "run command and catch out request: " << cmd
                     << std::endl;

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

std::string pathJoin(std::string base, std::string relative) {
  if (base.back() != '/') base += '/';
  if (relative[0] == '/') relative.erase(0, 1);
  base += relative;
  return base;
}

std::string pathBasename(const std::string_view entry) {
  char *base = basename(const_cast<char *>(entry.data()));
  return (base == nullptr) ? std::string() : std::string(base);
}

std::string pathDirname(const std::string_view entry) {
  char *base = dirname(const_cast<char *>(entry.data()));
  return (base == nullptr) ? std::string() : std::string(base);
}

bool changeMode(const std::string_view file, const mode_t mode) {
  LOGN(HELPER, INFO) << "change mode request: " << file << ". As mode: " << mode
                     << std::endl;
  return chmod(file.data(), mode) == 0;
}

bool changeOwner(const std::string_view file, const uid_t uid,
                 const gid_t gid) {
  LOGN(HELPER, INFO) << "change owner request: " << file
                     << ". As owner:group: " << uid << ":" << gid << std::endl;
  return chown(file.data(), uid, gid) == 0;
}

int openAndAddToCloseList(const std::string_view &path,
                          garbageCollector &collector, const int flags,
                          const mode_t mode) {
  const int fd =
      mode == 0 ? open(path.data(), flags) : open(path.data(), flags, mode);
  collector.closeAfterProgress(fd);
  return fd;
}

FILE *openAndAddToCloseList(const std::string_view &path,
                            garbageCollector &collector, const char *mode) {
  FILE *fp = fopen(path.data(), mode);
  collector.closeAfterProgress(fp);
  return fp;
}

DIR *openAndAddToCloseList(const std::string_view &path,
                           garbageCollector &collector) {
  DIR *dp = opendir(path.data());
  collector.closeAfterProgress(dp);
  return dp;
}

#ifdef __ANDROID__
std::string getProperty(const std::string_view prop) {
  char val[PROP_VALUE_MAX];
  const int x = __system_property_get(prop.data(), val);
  return x > 0 ? val : "ERROR";
}

bool reboot(const std::string_view arg) {
  LOGN(HELPER, INFO) << "reboot request sent!!!" << std::endl;

  unsigned cmd = ANDROID_RB_RESTART2;
  if (const std::string prop = getProperty("ro.build.version.sdk");
      prop != "ERROR") {
    if (std::stoi(prop) < 26) cmd = ANDROID_RB_RESTART;
  }

  return android_reboot(cmd, 0, arg.empty() ? nullptr : arg.data()) != -1;
}
#endif

uint64_t getRandomOffset(const uint64_t size, const uint64_t bufferSize) {
  if (size <= bufferSize) return 0;
  const uint64_t maxOffset = size - bufferSize;
  return rand() % maxOffset;
}

std::string convertTo(const uint64_t size, const std::string &multiple) {
  if (multiple == "KB") return std::to_string(TO_KB(size));
  if (multiple == "MB") return std::to_string(TO_MB(size));
  if (multiple == "GB") return std::to_string(TO_GB(size));
  return std::to_string(size);
}

std::string getLibVersion() { MKVERSION("libhelper"); }
} // namespace Helper
