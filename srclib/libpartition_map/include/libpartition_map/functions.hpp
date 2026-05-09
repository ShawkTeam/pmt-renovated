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
 * @file functions.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Functions of @c libpartition_map.
 */

#ifndef LIBPARTITION_MAP_FUNCTIONS_HPP
#define LIBPARTITION_MAP_FUNCTIONS_HPP

#if __cplusplus < 202002L
#error "libpartition_map/functions.hpp is requires C++20 or higher C++ standarts."
#endif

#include <string>
#include <libpartition_map/definations.hpp>

namespace PartitionMap {

/// @brief Get version string of library.
std::string getLibVersion();

namespace Extra {

/**
 * @brief Get version string of library.
 *
 * @return Version string of the library.
 */
std::string getLibVersion();

/**
 * @brief Get length of magic.
 *
 * @param magic Magic number.
 * @return Size of magic number.
 */
size_t getMagicLength(uint64_t magic);

/**
 * @brief Check if file has a magic number.
 *
 * @param magic Magic number.
 * @param buf File descriptor.
 * @param path File path.
 * @return True if file has the magic number, false otherwise.
 */
bool hasMagic(uint64_t magic, ssize_t buf, const std::string &path);

/**
 * @brief Format magic number.
 *
 * @param magic Magic number.
 * @return Formatted magic number string.
 */
std::string formatMagic(uint64_t magic);

/**
 * @brief Get size unit as string.
 *
 * @param size Size unit.
 * @return Size unit as string.
 */
std::string getSizeUnitAsString(SizeUnit size);

} // namespace Extra
} // namespace PartitionMap

#endif // #ifndef LIBPARTITION_MAP_FUNCTIONS_HPP
