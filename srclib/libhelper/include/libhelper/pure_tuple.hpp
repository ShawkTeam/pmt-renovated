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

#ifndef LIBHELPER_PURE_TUPLE_HPP
#define LIBHELPER_PURE_TUPLE_HPP

#if __cplusplus < 202002L
#error "libhelper/pure_tuple.hpp is requires C++20 or higher C++ standarts."
#endif

#include <string>
#include <tuple>
#include <initializer_list>
#include <algorithm>
#include <functional>

namespace Helper {
template <typename _Type1, typename _Type2, typename _Type3>
requires requires(_Type1 v1, _Type2 v2, _Type3 v3) {
  v1 = v1; v2 = v2; v3 = v3;
  v1 == v1 && v2 == v2 && v3 == v3;
} class PureTuple {
  void expand_if_needed() {
    if (count == capacity) {
      capacity *= 2;
      Data *data = new Data[capacity];

      for (size_t i = 0; i < count; i++)
        data[i] = tuple_data[i];

      delete[] tuple_data;
      tuple_data = data;
    }
  }

public:
  struct Data {
    _Type1 first;
    _Type2 second;
    _Type3 third;

    bool operator==(const std::tuple<_Type1, _Type2, _Type3> &t) const noexcept {
      return first == std::get<0>(t) && second == std::get<1>(t) && third == std::get<2>(t);
    }

    bool operator==(const Data &other) const noexcept {
      return first == other.first && second == other.second && third == other.third;
    }

    bool operator!=(const Data &other) const noexcept { return !(*this == other); }

    explicit operator bool() const noexcept { return first != _Type1{} || second != _Type2{} || third != _Type3{}; }

    bool operator!() const noexcept { return !bool{*this}; }

    void operator()(const std::tuple<_Type1, _Type2, _Type3> &t) {
      first = std::get<0>(t);
      second = std::get<1>(t);
      third = std::get<2>(t);
    }

    Data &operator=(const std::tuple<_Type1, _Type2, _Type3> &t) {
      first = std::get<0>(t);
      second = std::get<1>(t);
      third = std::get<2>(t);
      return *this;
    }
  };

  Data *tuple_data = nullptr;
  Data tuple_data_type = {_Type1{}, _Type2{}, _Type3{}};
  size_t capacity{}, count{};

  PureTuple() : tuple_data(new Data[20]), capacity(20), count(0) {}
  ~PureTuple() { delete[] tuple_data; }

  PureTuple(std::initializer_list<Data> val) : tuple_data(new Data[20]), capacity(20), count(0) {
    for (const auto &v : val)
      insert(v);
  }
  PureTuple(PureTuple &other) : tuple_data(new Data[other.capacity]), capacity(other.capacity), count(other.count) {
    std::copy(other.tuple_data, other.tuple_data + count, tuple_data);
  }
  PureTuple(PureTuple &&other) noexcept : tuple_data(new Data[other.capacity]), capacity(other.capacity), count(other.count) {
    std::copy(other.tuple_data, other.tuple_data + count, tuple_data);
    other.clear();
  }

  class iterator {
  private:
    Data *it;

  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Data;
    using difference_type = std::ptrdiff_t;
    using pointer = Data *;
    using reference = Data &;

    explicit iterator(Data *ptr) : it(ptr) {}

    pointer operator->() const { return it; }
    reference operator*() { return *it; }

    iterator &operator++() {
      ++it;
      return *this;
    }
    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    iterator &operator--() {
      --it;
      return *this;
    }
    iterator operator--(int) {
      iterator tmp = *this;
      --(*this);
      return tmp;
    }

    iterator &operator+=(difference_type n) {
      it += n;
      return *this;
    }
    iterator operator+(difference_type n) const { return iterator(it + n); }
    iterator &operator-=(difference_type n) {
      it -= n;
      return *this;
    }
    iterator operator-(difference_type n) const { return iterator(it - n); }
    difference_type operator-(const iterator &other) const { return it - other.it; }

    reference operator[](difference_type n) const { return it[n]; }

    bool operator<(const iterator &other) const { return it < other.it; }
    bool operator>(const iterator &other) const { return it > other.it; }
    bool operator<=(const iterator &other) const { return it <= other.it; }
    bool operator>=(const iterator &other) const { return it >= other.it; }

    bool operator!=(const iterator &other) const { return it != other.it; }
    bool operator==(const iterator &other) const { return it == other.it; }
  };

  bool find(const Data &data) const noexcept {
    for (size_t i = 0; i < count; i++)
      if (data == tuple_data[i]) return true;

    return false;
  }

  template <typename T = std::tuple<_Type1, _Type2, _Type3>>
    requires requires { std::is_same_v<T, std::tuple<_Type1, _Type2, _Type3>>; }
  bool find(const std::tuple<_Type1, _Type2, _Type3> &t) const noexcept {
    for (size_t i = 0; i < count; i++)
      if (tuple_data[i] == t) return true;

    return false;
  }

  bool find(const _Type1 &val, const _Type2 &val2, const _Type3 &val3) const noexcept {
    for (size_t i = 0; i < count; i++)
      if (tuple_data[i] == std::make_tuple(val, val2, val3)) return true;

    return false;
  }

  void insert(const Data &val) noexcept {
    expand_if_needed();
    if (!find(val)) tuple_data[count++] = val;
  }

  template <typename T = std::tuple<_Type1, _Type2, _Type3>>
    requires requires { std::is_same_v<T, std::tuple<_Type1, _Type2, _Type3>>; }
  void insert(const std::tuple<_Type1, _Type2, _Type3> &t) noexcept {
    expand_if_needed();
    if (!find(t)) tuple_data[count++] = Data{std::get<0>(t), std::get<1>(t), std::get<2>(t)};
  }

  void insert(const _Type1 &val, const _Type2 &val2, const _Type3 &val3) noexcept {
    expand_if_needed();
    if (!find(val, val2, val3)) tuple_data[count++] = Data{val, val2, val3};
  }

  void merge(const PureTuple &other) noexcept {
    for (const auto &v : other)
      insert(v);
  }

  void pop_back() noexcept {
    if (count > 0) --count;
  }

  void pop(const Data &data) noexcept {
    for (size_t i = 0; i < count; i++) {
      if (tuple_data[i] == data) {
        for (size_t j = i; j < count - 1; j++)
          tuple_data[j] = tuple_data[j + 1];
        --count;
        break;
      }
    }
  }

  void pop(const size_t i) noexcept {
    if (i >= count) return;
    for (size_t x = 0; x < count; x++) {
      if (i == x) {
        for (size_t j = i; j < count - 1; j++)
          tuple_data[j] = tuple_data[j + 1];
        --count;
        break;
      }
    }
  }

  void pop(const _Type1 &val, const _Type2 &val2, const _Type3 &val3) noexcept {
    for (size_t i = 0; i < count; i++) {
      if (tuple_data[i] == std::make_tuple(val, val2, val3)) {
        for (size_t j = i; j < count - 1; j++)
          tuple_data[j] = tuple_data[j + 1];
        --count;
        break;
      }
    }
  }

  template <typename T = std::tuple<_Type1, _Type2, _Type3>>
    requires requires { std::is_same_v<T, std::tuple<_Type1, _Type2, _Type3>>; }
  void pop(const std::tuple<_Type1, _Type2, _Type3> &t) noexcept {
    for (size_t i = 0; i < count; i++) {
      if (tuple_data[i] == t) {
        for (size_t j = i; j < count - 1; j++)
          tuple_data[j] = tuple_data[j + 1];
        --count;
        break;
      }
    }
  }

  void clear() noexcept {
    delete[] tuple_data;
    count = 0;
    capacity = 20;
    tuple_data = new Data[capacity];
  }

  Data back() const noexcept { return (count > 0) ? tuple_data[count - 1] : Data{}; }
  Data top() const noexcept { return (count > 0) ? tuple_data[0] : Data{}; }

  Data at(size_t i) const noexcept {
    if (i >= count) return Data{};
    return tuple_data[i];
  }

  void foreach (std::function<void(_Type1, _Type2, _Type3)> func) {
    for (size_t i = 0; i < count; i++)
      func(tuple_data[i].first, tuple_data[i].second, tuple_data[i].third);
  }

  void foreach (std::function<void(std::tuple<_Type1, _Type2, _Type3>)> func) {
    for (size_t i = 0; i < count; i++)
      func(std::make_tuple(tuple_data[i].first, tuple_data[i].second, tuple_data[i].third));
  }

  [[nodiscard]] size_t size() const noexcept { return count; }
  [[nodiscard]] bool empty() const noexcept { return count == 0; }

  iterator begin() const noexcept { return iterator(tuple_data); }
  iterator end() const noexcept { return iterator(tuple_data + count); }

  explicit operator bool() const noexcept { return count > 0; }
  bool operator!() const noexcept { return count == 0; }

  bool operator==(const PureTuple &other) const noexcept {
    if (this->count != other.count || this->capacity != other.capacity) return false;

    for (size_t i = 0; i < this->count; i++)
      if (tuple_data[i] != other.tuple_data[i]) return false;

    return true;
  }
  bool operator!=(const PureTuple &other) const noexcept { return !(*this == other); }

  Data operator[](size_t i) const noexcept {
    if (i >= count) return Data{};
    return tuple_data[i];
  }
  explicit operator int() const noexcept { return count; }

  PureTuple &operator=(const PureTuple &other) {
    if (this != &other) {
      delete[] tuple_data;

      capacity = other.capacity;
      count = other.count;
      tuple_data = new Data[capacity];

      std::copy(other.tuple_data, other.tuple_data + count, tuple_data);
    }

    return *this;
  }

  PureTuple &operator<<(const std::tuple<_Type1, _Type2, _Type3> &t) noexcept {
    insert(t);
    return *this;
  }

  friend PureTuple &operator>>(const std::tuple<_Type1, _Type2, _Type3> &t, PureTuple &tuple) noexcept {
    tuple.insert(t);
    return tuple;
  }
};

} // namespace Helper

#endif // #ifndef LIBHELPER_PURE_TUPLE_HPP
