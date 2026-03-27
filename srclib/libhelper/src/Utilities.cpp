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

#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <libhelper/functions.hpp>
#include <libhelper/management.hpp>
#ifndef ANDROID_BUILD
#include <sys/_system_properties.h>
#include <generated/buildInfo.hpp>
#else
#include <sys/system_properties.h>
#endif
#include <android-base/properties.h>
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

bool runCommand(const std::string& cmd) {
  LOGN(HELPER, INFO) << "run command request: " << cmd << std::endl;

  const std::array<const char *, 4> args = {
#ifdef __ANDROID__
      "/system/bin/sh",
#else
      "/bin/sh",
#endif
      "-c", cmd.data(), nullptr};
  pid_t pid = fork();

  if (pid < 0) return false;
  if (pid == 0) {
    execvp(args[0], const_cast<char *const *>(args.data()));
    _exit(127);
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) return false;

  return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
}

bool confirmPropt(const std::string &message, int maxTries = 10) {
  LOGN(HELPER, INFO) << "create confirm propt request." << std::endl;
  static int total_tries = 1;
  char p;

  printf("%s [ y / n ]: ", message.data());
  std::cin >> p;

  if (p == 'y' || p == 'Y') {
    total_tries = 1;
    return true;
  }
  if (p == 'n' || p == 'N') {
    total_tries = 1;
    return false;
  }

  if (total_tries >= maxTries) {
    printf("You have made many attempts (%d)!\n", maxTries);
    return false;
  }

  printf("Unexpected answer: '%c'. Try again.\n", p);
  total_tries++;
  return confirmPropt(message, maxTries);
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

#ifdef __ANDROID__
std::optional<std::string> getProperty(const std::string& prop) {
  auto &&val = android::base::GetProperty(prop, "ERROR");
  if (val == "ERROR") return std::nullopt;
  return val;
}

bool androidReboot(const std::string& arg) {
  LOGN(HELPER, INFO) << "reboot request sent!!!" << std::endl;

  unsigned cmd = ANDROID_RB_RESTART2;
  if (const auto& prop = getProperty("ro.build.version.sdk"); prop) {
    if (*prop != "ERROR" && std::stoi(*prop) < 26) cmd = ANDROID_RB_RESTART;
  }

  return android_reboot(cmd, 0, arg.empty() ? nullptr : arg.data()) != -1;
}
#endif

std::string multipleToString(const sizeCastTypes type) {
  if (type == KB) return "KB";
  if (type == MB) return "MB";
  if (type == GB) return "GB";
  return "B";
}

std::string getLibVersion() { MKVERSION("libhelper"); }
} // namespace Helper
