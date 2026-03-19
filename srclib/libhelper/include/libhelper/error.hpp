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

#ifndef LIBHELPER_ERROR_HPP
#define LIBHELPER_ERROR_HPP

#include <exception>
#include <sstream>
#include <string>
#include <format>
#include <libhelper/logging.hpp>

namespace Helper {
// Throwable error class
class Error final : public std::exception {
  std::ostringstream oss;
  std::string message;

public:
  Error() = default;
  Error(const Error &other) noexcept : message(other.message) {}

  template <typename... Args> explicit Error(std::format_string<Args...> fmt, Args &&...args) {
    oss << std::format(fmt, std::forward<Args>(args)...);
    message = oss.str();
  }

  template <typename T> Error &&operator<<(const T &msg) && {
    oss << msg;
    message = oss.str();
    return std::move(*this);
  }

  Error &operator<<(std::ostream &(*msg)(std::ostream &)) {
    oss << msg;
    message = oss.str();
    return *this;
  }

  [[nodiscard]] const char *what() const noexcept override { return message.data(); }
};

} // namespace Helper

#endif // #ifndef LIBHELPER_ERROR_HPP
