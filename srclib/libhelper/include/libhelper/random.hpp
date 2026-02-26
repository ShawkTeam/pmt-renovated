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

#ifndef LIBHELPER_RANDOM_HPP
#define LIBHELPER_RANDOM_HPP

#include <set>
#include <random>

namespace Helper {

template <int max = 100, int start = 0, int count = 10, int d = 0> class Random {
  static_assert(max > start, "max is larger than start");
  static_assert(count > 1, "count is larger than 1");
  static_assert(count <= max - start, "count is greater than max-start");

public:
  static std::set<int> get() {
    std::set<int> set;
    std::random_device rd;
    std::mt19937 gen(rd());

    if constexpr (d > 0) {
      std::uniform_int_distribution dist(0, (max - start - 1) / d);
      while (set.size() < count)
        set.insert(start + dist(gen) * d);
    } else {
      std::uniform_int_distribution dist(start, max - 1);
      while (set.size() < count)
        set.insert(dist(gen));
    }

    return set;
  }

  static int getNumber() {
    std::random_device rd;
    std::mt19937 gen(rd());
    int ret;

    if constexpr (d > 0) {
      std::uniform_int_distribution dist(0, (max - start - 1) / d);
      ret = start + dist(gen) * d;
    } else {
      std::uniform_int_distribution dist(start, max - 1); // max exclusive
      ret = dist(gen);
    }

    return ret;
  }
};

} // namespace Helper

#endif // #ifndef LIBHELPER_RANDOM_HPP
