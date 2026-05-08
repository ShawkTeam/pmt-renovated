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
 * @file capsule.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Provides a capsule structure to store variable references and values.
 */

#ifndef LIBHELPER_CAPSULE_HPP
#define LIBHELPER_CAPSULE_HPP

namespace Helper {

/**
 * @brief A "capsule" class that stores references.
 *
 * @tparam _Type Contained value type. Must support copy assignment and equality comparison operators.
 * @note Uses C++20 requires clause to enforce type constraints at compile time.
 */
template <typename _Type>
  requires requires(_Type v) {
    v = v;
    v == v;
  }
class Capsule {
public:
  _Type &value;

  /**
   * @brief A constructor that takes data as a reference.
   *
   * @note It certainly doesn't have to be used. Take a look at the @ref set() function.
   * @see set()
   */
  explicit Capsule(_Type &value) noexcept : value(value) {}

  /**
   * @brief Get a reference to another piece of data and save it. Often, you can make changes later as well.
   *
   * @param _value Input variable.
   */
  void set(_Type &_value) noexcept { this->value = _value; }

  /**
   * @brief Retrieve the stored data.
   *
   * @return Reference of capsuled variable.
   */
  _Type &get() noexcept { return this->value; }

  /**
   * @brief Retrieve the stored data.
   *
   * @return Const reference of capsuled variable.
   */
  _Type &get() const noexcept { return this->value; }

  /**
   * @brief Get the reference held directly with the '&' operator.
   *
   * @return Reference of capsuled variable.
   */
  explicit operator _Type &() noexcept { return &this->value; }

  /**
   * @brief Get the reference held directly with the '&' operator.
   *
   * @return Const reference of capsuled variable.
   */
  explicit operator _Type &() const noexcept { return &this->value; }

  /**
   * @brief Get the reference held directly with the '*' operator.
   *
   * @return Reference of capsuled variable.
   */
  explicit operator _Type *() noexcept { return &this->value; }

  /**
   * @brief Take the reference held by another capsule.
   *
   * @param other Other capsule class.
   */
  Capsule &operator=(const Capsule &other) noexcept = default;

  /**
   * @brief Change the held reference.
   *
   * @param _value Input variable.
   */
  Capsule &operator=(const _Type &_value) noexcept {
    this->value = _value;
    return *this;
  }

  /**
   * @brief Check if the data held by the input capsule matches the data being held (==).
   *
   * @param other Other capsule class.
   * @retval true  The data held is the same.
   * @retval false The data held is the NOT same.
   */
  bool operator==(const Capsule &other) const noexcept { return this->value == other.value; }

  /**
   * @brief Check if the input data matches the stored data (==).
   *
   * @param other Other capsule class.
   * @retval true  The data held is the same.
   * @retval false The data held is the NOT same.
   */
  bool operator==(const _Type &_value) const noexcept { return this->value == _value; }

  /**
   * @brief Check if the data held by the input capsule matches the data being held (!=).
   *
   * @param other Other capsule class.
   * @retval true  The data held is the NOT same.
   * @retval false The data held is the same.
   */
  bool operator!=(const Capsule &other) const noexcept { return *this != other; }

  /**
   * @brief Check if the input data matches the stored data (!=).
   *
   * @param other Other capsule class.
   * @retval true  The data held is the NOT same.
   * @retval false The data held is the same.
   */
  bool operator!=(const _Type &_value) const noexcept { return *this != _value; }

  /**
   * @brief Check if the reference data being held is the default (bool()).
   *
   * @retval true  HeldData != _Type{}
   * @retval false HeldData = _Type{}
   */
  explicit operator bool() const noexcept { return this->value != _Type{}; }

  /**
   * @brief Check if the reference data being held is the default (!()).
   *
   * @retval true  HeldData = _Type{}
   * @retval false HeldData != _Type{}
   */
  bool operator!() const noexcept { return this->value == _Type{}; }

  /**
   * @brief Change the held reference (with C++ style).
   *
   * @param _value Input variable.
   */
  friend Capsule &operator>>(const _Type &_value, Capsule &_capsule) noexcept {
    _capsule.value = _value;
    return _capsule;
  }

  /// @brief Get the held reference.
  _Type &operator()() noexcept { return value; }

  /// @brief Get the held reference.
  _Type &operator()() const noexcept { return value; }

  /**
   * @brief Change the held reference.
   *
   * @param _value Input variable.
   */
  void operator()(const _Type &_value) noexcept { this->value = _value; }
};

} // namespace Helper

#endif // #ifndef LIBHELPER_CAPSULE_HPP
