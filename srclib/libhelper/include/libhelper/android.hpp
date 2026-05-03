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

#ifndef LIBHELPER_ANDROID_HPP
#define LIBHELPER_ANDROID_HPP

#include <string>
#include <array>
#include <optional>
#include <set>
#include <private/android_filesystem_config.h>

#ifdef __ANDROID__

namespace Helper::Android {

inline constexpr const char *BINARY_SU = "su";
inline constexpr const char *BINARY_BUSYBOX = "busybox";

inline constexpr std::array<std::string_view, 15> KNOWN_SU_BINARY_PATHS = {"/data/local/",
                                                                           "/data/local/bin/",
                                                                           "/data/local/xbin/",
                                                                           "/sbin/",
                                                                           "/su/bin/",
                                                                           "/system/bin/",
                                                                           "/system/bin/.ext/",
                                                                           "/system/bin/failsafe/",
                                                                           "/system/sd/xbin/",
                                                                           "/system/usr/we-need-root/",
                                                                           "/system/xbin/",
                                                                           "/system_ext/bin/",
                                                                           "/cache/",
                                                                           "/data/",
                                                                           "/dev/"};

inline constexpr std::array<std::string_view, 7> KNOWN_MAYBE_NOT_WRITABLE_PATHS = {
    "/system", "/system/bin", "/system/sbin", "/system/xbin", "/vendor/bin", "/sbin", "/etc"};

/**
 * Get PATH variable as a splitted list (as std::set).
 */
std::set<std::string> getPaths();

/**
 * Get input property as string (for Android).
 * Returns std::nullopt on any error.
 */
std::optional<std::string> getProperty(const std::string &property);

/**
 * Reboot device to input mode (for Android).
 */
bool reboot(const std::string &arg);

/**
 * Search su binary on device.
 */
bool isRooted();

/**
 * It is checked whether the user ID used is equivalent to AID_ROOT.
 * See external/core/libcutils/include/private/android_filesystem_config.h
 */
inline bool isHasRootPrivileges(uid_t uid = AID_ROOT) { return getuid() == uid; }

/**
 * It is checked whether the user ID used is equivalent to AID_SHELL.
 * See external/core/libcutils/include/private/android_filesystem_config.h
 */
inline bool isHasAdbPrivileges(uid_t uid = AID_SHELL) { return getuid() == uid; }
} // namespace Helper::Android

#endif // #ifdef __ANDROID__
#endif // #ifndef LIBHELPER_ANDROID_HPP
