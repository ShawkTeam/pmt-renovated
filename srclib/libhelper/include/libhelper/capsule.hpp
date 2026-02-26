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

#ifndef LIBHELPER_CAPSULE_HPP
#define LIBHELPER_CAPSULE_HPP

#include <libhelper/management.hpp>

namespace Helper {
// Provides a capsule structure to store variable references and values.
template <typename _Type> class Capsule : public garbageCollector {
public:
  _Type &value;

  // The value to be stored is taken as a reference as an argument
  explicit Capsule(_Type &value) noexcept : value(value) {}

  // Set the value.
  void set(_Type &_value) noexcept { this->value = _value; }

  // Get reference of the value.
  _Type &get() noexcept { return this->value; }
  _Type &get() const noexcept { return this->value; }

  // You can get the reference of the stored value in the input type (casting is
  // required).
  explicit operator _Type &() noexcept { return &this->value; }
  explicit operator _Type &() const noexcept { return &this->value; }
  explicit operator _Type *() noexcept { return &this->value; }

  // The value of another capsule is taken.
  Capsule &operator=(const Capsule &other) noexcept {
    this->value = other.value;
    return *this;
  }

  // Assign another value.
  Capsule &operator=(const _Type &_value) noexcept {
    this->value = _value;
    return *this;
  }

  // Check if this capsule and another capsule hold the same data.
  bool operator==(const Capsule &other) const noexcept { return this->value == other.value; }

  // Check if this capsule value and another capsule value hold the same data.
  bool operator==(const _Type &_value) const noexcept { return this->value == _value; }

  // Check that this capsule and another capsule do not hold the same data.
  bool operator!=(const Capsule &other) const noexcept { return !(*this == other); }

  // Check that this capsule value and another capsule value do not hold the
  // same data.
  bool operator!=(const _Type &_value) const noexcept { return !(*this == _value); }

  // Check if the current held value is actually empty.
  explicit operator bool() const noexcept { return this->value != _Type{}; }

  // Check that the current held value is actually empty.
  bool operator!() const noexcept { return this->value == _Type{}; }

  // Change the value with the input operator.
  friend Capsule &operator>>(const _Type &_value, Capsule &_capsule) noexcept {
    _capsule.value = _value;
    return _capsule;
  }

  // Get the reference of the value held.
  _Type &operator()() noexcept { return value; }
  _Type &operator()() const noexcept { return value; }

  // Set the value.
  void operator()(const _Type &_value) noexcept { this->value = _value; }
};

} // namespace Helper

#endif // #ifndef LIBHELPER_CAPSULE_HPP
