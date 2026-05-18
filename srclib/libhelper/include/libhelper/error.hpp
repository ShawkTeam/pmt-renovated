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
 * @file error.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief It offers a modern throwable error class.
 */

#ifndef LIBHELPER_ERROR_HPP
#define LIBHELPER_ERROR_HPP

#include <exception>
#include <sstream>
#include <string>
#include <format>
#include <libhelper/logging.hpp>

namespace Helper {

/**
 * @brief A modern, throwable error class. It provides the @c << operator and offers usage in the @c std::print style of modern C++23.
 *
 * @code
 * void someFunction(void) {
 *   int e = Helper::Random<>::getNumber();
 *   // I will ignore the namespace in the instance of the 'Error' class.
 *
 *   throw Error() << "An error occurred! Error code: " << e; // With << operator.
 *   throw Error().withCode(e) << "An error code occurred!"; // With << operator and error code specifying.
 *   throw Error("An error occurred! Error code: {}", e); // With <format>.
 *   throw Error("An error occurred!").withCode(e); // With <format> and error code specifying.
 *
 *   // Let's make things a little more interesting...
 *   throw Error("What is that? ").withCode(e) << "Oh right, that's a error!";
 * }
 *
 * int main(int argc, char** argv) {
 *   try {
 *      someFunction();
 *   } catch (Error &e) {
 *      // Let's assume we receive the "interesting" error message we wrote at the end.
 *      std::cout << e.what() << " Error code: " << e.getErrorCode() << std::endl;
 *      return e.getErrorCode();
 *   }
 *
 *   return 0;
 * }
 * @endcode
 */
class Error final : public std::exception {
  std::ostringstream oss;
  std::string message;
  int ec = 1;
  bool is_cmdline_error = false;

public:
  Error() = default;

  /// @brief Copy constructor.
  Error(const Error &other) noexcept : message(other.message) {}

  /// @brief Modern std::print style input field constructor.
  template <typename... Args> explicit Error(std::format_string<Args...> fmt, Args &&...args) {
    oss << std::format(fmt, std::forward<Args>(args)...);
    message = oss.str();
  }

  /// @brief To use the @c << operator for receiving non-function-like inputs.
  template <typename T> Error &&operator<<(const T &msg) && {
    oss << msg;
    message = oss.str();
    return std::move(*this);
  }

  /// @brief To receive function-like inputs, use the @c << operator.
  Error &operator<<(std::ostream &(*msg)(std::ostream &)) {
    oss << msg;
    message = oss.str();
    return *this;
  }

  /// @brief It is used to determine the error code when an error is thrown.
  Error &&withCode(int _ec) {
    ec = _ec;
    return std::move(*this);
  }

  /// @brief Set is_cmdline_error value (private).
  Error &&cmdlineError(bool v = true) {
    is_cmdline_error = v;
    return std::move(*this);
  }

  /// @brief Get error code.
  int getErrorCode() const { return ec; }

  /// @brief Get is_cmdline_error value (private).
  bool isCmdlineError() const { return is_cmdline_error; }

  /// @brief Get error message.
  [[nodiscard]] const char *what() const noexcept override { return message.data(); }
};

} // namespace Helper

#endif // #ifndef LIBHELPER_ERROR_HPP
