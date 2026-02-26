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
#include <string_view>

namespace Helper {
// -------------------------------
// Android - not throws Helper::Error
// -------------------------------
#ifdef __ANDROID__
/**
 * Get input property as string (for Android).
 * Returns "ERROR" on any error.
 */
std::string getProperty(std::string_view prop);

/**
 * Reboot device to input mode (for Android).
 */
bool androidReboot(std::string_view arg);
#endif // #ifdef __ANDROID__

} // namespace Helper

#endif // #ifndef LIBHELPER_ANDROID_HPP
