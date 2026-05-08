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

/**
 * @file android.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Android-specific system utilities for root detection, property access and device control.
 */

#ifndef LIBHELPER_ANDROID_HPP
#define LIBHELPER_ANDROID_HPP

#include <string>
#include <array>
#include <optional>
#include <set>
#include <private/android_filesystem_config.h>

#ifdef __ANDROID__

/**
 * @namespace Helper::Android
 * @brief Utilities for Android environment interaction.
 *
 * Provides functions for root detection, system property retrieval,
 * device reboot, and privilege checking via Android UID system.
 * Only available when compiled for Android targets.
 */
namespace Helper::Android {

/// @brief Name of the su binary used for root detection.
inline constexpr const char *BINARY_SU = "su";

/// @brief Name of the busybox binary.
inline constexpr const char *BINARY_BUSYBOX = "busybox";

/**
 * @brief Known filesystem paths where the su binary may reside.
 *
 * Used by @ref isRooted() to search for root access on the device.
 */
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

/**
 * @brief Filesystem paths that may not be writable even with root.
 *
 * These paths are typically read-only mounted partitions.
 */
inline constexpr std::array<std::string_view, 7> KNOWN_MAYBE_NOT_WRITABLE_PATHS = {
    "/system", "/system/bin", "/system/sbin", "/system/xbin", "/vendor/bin", "/sbin", "/etc"};

/**
 * @brief Returns the PATH environment variable as a set of individual paths.
 * @return A set of path strings parsed from the PATH variable.
 */
std::set<std::string> getPaths();

/**
 * @brief Retrieves an Android system property by name.
 * @param property The property key (e.g. "ro.build.version.release").
 * @return The property value as string, or @c std::nullopt on any error.
 */
std::optional<std::string> getProperty(const std::string &property);

/**
 * @brief Reboots the device into the specified mode.
 * @param arg Reboot mode argument (e.g. "recovery", "bootloader", "").
 * @return @c true if the reboot command was issued successfully, @c false otherwise.
 */
bool reboot(const std::string &arg);

/**
 * @brief Checks whether the device is rooted by searching for su binary.
 *
 * Iterates over @ref KNOWN_SU_BINARY_PATHS to find a valid su binary.
 *
 * @return @c true if su binary is found, @c false otherwise.
 */
bool isRooted();

/**
 * @brief Checks whether the current process has root privileges.
 *
 * Compares the effective UID against AID_ROOT.
 * See @c external/core/libcutils/include/private/android_filesystem_config.h
 *
 * @param uid UID to compare against (default: AID_ROOT).
 * @return @c true if current UID matches @p uid.
 */
inline bool isHasRootPrivileges(uid_t uid = AID_ROOT) { return getuid() == uid; }

/**
 * @brief Checks whether the current process has ADB (shell) privileges.
 *
 * Compares the effective UID against AID_SHELL.
 * See @c external/core/libcutils/include/private/android_filesystem_config.h
 *
 * @param uid UID to compare against (default: AID_SHELL).
 * @return @c true if current UID matches @p uid.
 */
inline bool isHasAdbPrivileges(uid_t uid = AID_SHELL) { return getuid() == uid; }

} // namespace Helper::Android

#endif // __ANDROID__
#endif // LIBHELPER_ANDROID_HPP