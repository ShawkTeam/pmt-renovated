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
#include <optional>
#include <private/android_filesystem_config.h>

namespace Helper {
// -------------------------------
// Android - not throws Helper::Error
// -------------------------------
#ifdef __ANDROID__
/**
 * Get input property as string (for Android).
 * Returns std::nullopt on any error.
 */
std::optional<std::string> getProperty(const std::string& prop);

/**
 * Reboot device to input mode (for Android).
 */
bool androidReboot(const std::string& arg);

/**
 * It is checked whether the user ID used is equivalent to AID_ROOT.
 * See external/core/libcutils/include/private/android_filesystem_config.h
 */
inline bool hasSuperUser(uid_t uid = AID_ROOT) { return getuid() == uid; }

/**
 * It is checked whether the user ID used is equivalent to AID_SHELL.
 * See external/core/libcutils/include/private/android_filesystem_config.h
 */
inline bool hasAdbPermissions(uid_t uid = AID_SHELL) { return getuid() == uid; }
#endif // #ifdef __ANDROID__

} // namespace Helper

#endif // #ifndef LIBHELPER_ANDROID_HPP
