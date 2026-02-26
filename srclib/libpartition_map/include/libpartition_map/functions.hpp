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

#ifndef LIBPARTITION_MAP_FUNCTIONS_HPP
#define LIBPARTITION_MAP_FUNCTIONS_HPP

#if __cplusplus < 202002L
#error "libpartition_map/functions.hpp is requires C++20 or higher C++ standarts."
#endif

#include <string>
#include <libpartition_map/definations.hpp>

namespace PartitionMap {

std::string getLibVersion(); // Get version string of library.

namespace Extra {

size_t getMagicLength(uint64_t magic);
bool hasMagic(uint64_t magic, ssize_t buf, const std::string &path);
std::string formatMagic(uint64_t magic);
std::string getSizeUnitAsString(SizeUnit size);

} // namespace Extra
} // namespace PartitionMap

#endif // #ifndef LIBPARTITION_MAP_FUNCTIONS_HPP
