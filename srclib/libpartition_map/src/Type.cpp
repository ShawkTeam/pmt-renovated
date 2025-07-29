/*
   Copyright 2025 Yağız Zengin

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <string>
#include <cstdint>
#include <utility>
#include <libpartition_map/lib.hpp>

namespace PartitionMap {

basic_partition_map::iterator::iterator(_entry* p) : ptr(p) {}

auto basic_partition_map::iterator::operator*() -> std::pair<std::string&, decltype(_entry::props)&> 
{
	return {ptr->name, ptr->props};
}

_entry* basic_partition_map::iterator::operator->()
{
	return ptr;
}

basic_partition_map::iterator& basic_partition_map::iterator::operator++()
{
	++ptr;
	return *this;
}

basic_partition_map::iterator basic_partition_map::iterator::operator++(int)
{
	basic_partition_map::iterator tmp = *this;
	++ptr;
	return tmp;
}

bool basic_partition_map::iterator::operator==(const basic_partition_map::iterator& other) const
{
	return ptr == other.ptr;
}

bool basic_partition_map::iterator::operator!=(const basic_partition_map::iterator& other) const
{
	return ptr != other.ptr;
}

basic_partition_map::constant_iterator::constant_iterator(const _entry* p) : ptr(p) {}

auto basic_partition_map::constant_iterator::operator*() const -> std::pair<const std::string&, const decltype(_entry::props)&> {
	return {ptr->name, ptr->props};
}


const _entry* basic_partition_map::constant_iterator::operator->() const
{
	return ptr;
}

basic_partition_map::constant_iterator& basic_partition_map::constant_iterator::operator++()
{
	++ptr;
	return *this;
}

basic_partition_map::constant_iterator basic_partition_map::constant_iterator::operator++(int)
{
	basic_partition_map::constant_iterator tmp = *this;
	++ptr;
	return tmp;
}

bool basic_partition_map::constant_iterator::operator==(const basic_partition_map::constant_iterator& other) const
{
	return ptr == other.ptr;
}

bool basic_partition_map::constant_iterator::operator!=(const basic_partition_map::constant_iterator& other) const
{
	return ptr != other.ptr;
}

void basic_partition_map::_resize_map()
{
	size_t new_capacity = _capacity * 2;
	_entry* new_data = new _entry[new_capacity];

	for (size_t i = 0; i < _count; i++) new_data[i] = _data[i];

	delete[] _data;
	_data = new_data;
	_capacity = new_capacity;
}

int basic_partition_map::_index_of(const std::string_view name) const
{
	for (size_t i = 0; i < _count; i++) {
		if (name == _data[i].name) return (int)i;
	}

	return 0;
}

basic_partition_map::basic_partition_map(const std::string name, uint64_t size, bool logical)
{
	_data = new _entry[_capacity];
	insert(name, size, logical);
}

basic_partition_map::basic_partition_map(const basic_partition_map& other) : 
    _data(new _entry[other._capacity]),
    _count(other._count),
    _capacity(other._capacity)
{
	std::copy(other._data, other._data + _count, _data);
}

basic_partition_map::basic_partition_map() : _count(0), _capacity(6)
{
	_data = new _entry[_capacity];
}

basic_partition_map::~basic_partition_map()
{
	delete[] _data;
}

bool basic_partition_map::insert(const std::string name, uint64_t size, bool logical)
{
	if (name == _data[_index_of(name)].name) return false;
	if (_count == _capacity) _resize_map();

	_data[_count++] = {name, {size, logical}};
	LOGN(MAP, INFO) << std::boolalpha << "partition " << name << " inserted (size=" << size << ", is_logical=" << logical << ")." << std::endl;
	return true;
}

void basic_partition_map::merge(const basic_partition_map& map)
{
	LOGN(MAP, INFO) << "map merge request." << std::endl;
	for (const auto& [name, props] : map)
		insert(name, props.size, props.isLogical);
	LOGN(MAP, INFO) << "map merged successfully." << std::endl;
}

uint64_t basic_partition_map::get_size(const std::string_view name) const
{
	int pos = _index_of(name);
	if (name == _data[pos].name) return _data[pos].props.size;

	return 0;
}

bool basic_partition_map::is_logical(const std::string_view name) const
{
	int pos = _index_of(name);
	if (name == _data[pos].name) return _data[pos].props.isLogical;

	return false;
}

basic_partition_map::_returnable_entry basic_partition_map::get_all(const std::string_view name) const
{
	int pos = _index_of(name);
	if (name == _data[pos].name) 
		return _returnable_entry{_data[pos].props.size, _data[pos].props.isLogical};

	return _returnable_entry{};
}

bool basic_partition_map::find(const std::string_view name) const
{
	if (name == _data[_index_of(name)].name) return true;

	return false;
}

std::string basic_partition_map::find_(const std::string name) const
{
	if (name == _data[_index_of(name)].name) return name;

	return {};
}

size_t basic_partition_map::size() const
{
	return _count;
}

bool basic_partition_map::empty() const
{
	if (_count > 0) return false;
	return true;
}

void basic_partition_map::clear()
{
	LOGN(MAP, INFO) << "map clean requested. Map is empty now." << std::endl;
	delete[] _data;
	_count = 0;
	_capacity = 6;
	_data = new _entry[_capacity];
}

basic_partition_map& basic_partition_map::operator=(const basic_partition_map& map)
{
	if (this != &map) {
		delete[] _data;

		_capacity = map._capacity;
		_count = map._count;
		_data = new _entry[_capacity];
		std::copy(map._data, map._data + _count, _data);
	}

	return *this;
}

bool basic_partition_map::operator==(const basic_partition_map& other) const
{
	if (this->_capacity != other._capacity
	    || this->_count != other._count)
		return false;

	for (size_t i = 0; i < _count; i++)
		if (_data[i].name == other._data[i].name
		    && _data[i].props.size == other._data[i].props.size
		    && _data[i].props.isLogical == other._data[i].props.isLogical)
			continue;
		else
			return false;

	return true;
}

bool basic_partition_map::operator!=(const basic_partition_map& other) const
{
	return !(*this == other);
}

basic_partition_map::iterator basic_partition_map::begin()
{
	return basic_partition_map::iterator(_data);
}

basic_partition_map::iterator basic_partition_map::end()
{
	return basic_partition_map::iterator(_data + _count);
}

basic_partition_map::constant_iterator basic_partition_map::begin() const
{
	return basic_partition_map::constant_iterator(_data);
}

basic_partition_map::constant_iterator basic_partition_map::cbegin() const
{
	return basic_partition_map::constant_iterator(_data);
}

basic_partition_map::constant_iterator basic_partition_map::end() const
{
	return basic_partition_map::constant_iterator(_data + _count);
}

basic_partition_map::constant_iterator basic_partition_map::cend() const
{
	return basic_partition_map::constant_iterator(_data + _count);
}

} // namespace PartitionMap
