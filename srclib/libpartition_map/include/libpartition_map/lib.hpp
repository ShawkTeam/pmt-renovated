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

#ifndef LIBPARTITION_MAP_LIB_HPP
#define LIBPARTITION_MAP_LIB_HPP

#include <string>
#include <string_view>
#include <optional>
#include <exception>
#include <list>
#include <memory>
#include <utility> // for std::pair
#include <cstdint> // for uint64_t
#include <libhelper/lib.hpp>

namespace PartitionMap {

struct _entry {
	std::string name;

	struct {
		uint64_t size;
		bool isLogical;
	} props;
};

/**
 * basic_partition_map
 * -------------------
 *   The main type of the library. The Builder class is designed
 *   to be easily manipulated and modified only on this class.
 */
class basic_partition_map {
private:
	void _resize_map();
	int _index_of(const std::string_view name) const;

public:
	_entry* _data;
	size_t _count, _capacity;

	struct _returnable_entry {
		uint64_t size;
		bool isLogical;
	};

	using BasicInf = _returnable_entry;

	basic_partition_map(const std::string name, uint64_t size, bool logical);
	basic_partition_map(const basic_partition_map& other);
	basic_partition_map();
	~basic_partition_map();

	bool insert(const std::string name, uint64_t size, bool logical);
	void merge(const basic_partition_map& map);
	uint64_t get_size(const std::string_view name) const;
	bool is_logical(const std::string_view name) const;
	_returnable_entry get_all(const std::string_view name) const;
	bool find(const std::string_view name) const;
	std::string find_(const std::string name) const;
	size_t size() const;
	bool empty() const;
	void clear();

	basic_partition_map& operator=(const basic_partition_map& map);
	bool operator==(const basic_partition_map& other) const;
	bool operator!=(const basic_partition_map& other) const;

	class iterator {
	public:
		_entry* ptr;

		iterator(_entry* p);

		auto operator*() -> std::pair<std::string&, decltype(_entry::props)&>;
		_entry* operator->();
		iterator& operator++();
		iterator operator++(int);
		bool operator!=(const iterator& other) const;
		bool operator==(const iterator& other) const;
	};

	class constant_iterator {
	public:
		const _entry* ptr;

		constant_iterator(const _entry* p);

		auto operator*() const -> std::pair<const std::string&, const decltype(_entry::props)&>;
		const _entry* operator->() const;
		constant_iterator& operator++();
		constant_iterator operator++(int);
		bool operator!=(const constant_iterator& other) const;
		bool operator==(const constant_iterator& other) const;
	};

	/* for-each support */
	iterator begin();
	iterator end();

	constant_iterator begin() const;
	constant_iterator cbegin() const;
	constant_iterator end() const;
	constant_iterator cend() const;
};

using Map_t = basic_partition_map;

class basic_partition_map_builder {
private:
	Map_t _current_map;
	std::string _workdir;
	bool _any_generating_error, _map_builded;

	bool _is_real_block_dir(const std::string_view path) const;
	Map_t _build_map(std::string_view path, bool logical = false);
	void _insert_logicals(Map_t&& logicals);
	void _map_build_check() const;
	uint64_t _get_size(const std::string path);

public:
	/**
	 * Default constructor
	 * -------------------
	 *   By default, it searches the directories in the
	 *   defaultEntryList in PartitionMap.cpp in order and
	 *   uses the directory it finds.
	 */
	basic_partition_map_builder();

	/**
	 * Secondary constructor
	 * ---------------------
	 *   It has one arguments:
	 *      - Directory path to search
	 */
	basic_partition_map_builder(const std::string_view path);

	/**
	 * getAll()
	 * --------
	 *   Returns the current list content in Map_t type.
	 *   If no list is created, returns std::nullopt.
	 */
	Map_t getAll() const;

	/**
	 * get(name)
	 * ---------
	 *   WARNING: Learn about std::optional before using this function.
	 * 
	 *   Returns information of a specific partition in
	 *   Map_temp_t type. If the partition is not in the
	 *   currently created list, returns std::nullopt.
	 */
	std::optional<std::pair<uint64_t, bool>> get(const std::string_view name) const;

	/**
	 * getLogicalPartitionList()
	 * -------------------------
	 *   WARNING: Learn about std::optional before using this function.
	 * 
	 *   If there is a logical partition(s) in the created
	 *   list, it returns a list of type std::list (containing
	 *   data of type std::string). If there is no logical
	 *   partition in the created list, it returns std::nullopt.
	 */
	std::optional<std::list<std::string>> getLogicalPartitionList() const;

	/**
	 * getPhysicalPartitionList()
	 * --------------------------
	 *   WARNING: Learn about std::optional before using this function.
	 * 
	 *   The physical partitions in the created list are
	 *   returned as std::list type. If there is no content
	 *   due to any problem, returns std::nullopt.
	 */
	std::optional<std::list<std::string>> getPhysicalPartitionList() const;

	/**
	 * getRealLinkPathOf(name)
	 * -----------------------
	 *   WARNING: Learn about std::optional before using this function.
	 * 
	 *   Returns the full link path of the entered partition
	 *   name in the current search directory as std::string.
	 *   If the partition is not in the list, an empty
	 *   std::string is returned.
	 */
	std::string getRealLinkPathOf(const std::string_view name) const;

	/**
	 * getRealPathOf(name)
	 * -------------------
	 *   WARNING: Learn about std::optional before using this function.
	 * 
	 *   Returns the actual path of the partition as 
	 *   std::string. Like /dev/block/sda5
	 */
	 std::string getRealPathOf(const std::string_view name) const;

	/**
	 * getCurrentWorkDir()
	 * -------------------
	 *   WARNING: Learn about std::optional before using this function.
	 * 
	 *   If it exists, the path to the search string is
	 *   returned as std::string. If it does not exist,
	 *   an empty std::string is returned.
	 */
	std::string getCurrentWorkDir() const;

	/**
	 * hasPartition(name)
	 * ------------------
	 *   Returns whether the entered partition name is in the
	 *   created partition list as a bool.
	 */
	bool hasPartition(const std::string_view name) const;

	/**
	 * isLogical(name)
	 * ---------------
	 *   Returns the bool type status of whether the
	 *   entered section name is marked as logical in the
	 *   created list. Alternatively, the current section
	 *   information can be retrieved with the Get() function
	 *   and checked for logicality.
	 */
	bool isLogical(const std::string_view name) const;

	/**
	 * clear()
	 * -------
	 *   The created list and the current search index name are cleared. 
	 */
	void clear();

	/**
	 * readDirectory(path)
	 * -------------------
	 *   The entered path is defined as the new search
	 *   directory and the search is performed in the entered
	 *   directory. If everything goes well, true is returned.
	 */
	bool readDirectory(const std::string_view path);

	/**
	 * empty()
	 * -------
	 *   Whether the current list is empty or not is returned
	 *   as bool type. If there is content in the list, true
	 *   is returned, otherwise false is returned.
	 */
	bool empty() const;

	/**
	 * sizeOf(name)
	 * ------------
	 *   WARNING: Learn about std::optional before using this function.
	 * 
	 *   If it exists, the size of the partition with the
	 *   entered name is returned as uint64_t type.
	 *   If it does not exist, 0 is returned.
	 */
	uint64_t sizeOf(const std::string_view name) const;

	/**
	 * == operator
	 * -----------
	 *   If the content lists of the two created objects are
	 *   the same (checked only according to the partition
	 *   names), true is returned, otherwise false is returned
	 */
	friend bool operator==(basic_partition_map_builder& lhs, basic_partition_map_builder& rhs);

	/**
	 * != operator
	 * -----------
	 *   The opposite logic of the == operator.
	 */
	friend bool operator!=(basic_partition_map_builder& lhs, basic_partition_map_builder& rhs);

	/**
	 * Boolean operator
	 * ----------------
	 *   You can check whether the object was created
	 *   successfully. If the problem did not occur, true is
	 *   returned, if it did, false is returned.
	 */
	operator bool() const;

	/**
	 * ! operator
	 * ----------
	 *   Returns true if the object creation failed (i.e., there's a problem),
	 *   and false if the object is correctly created.
	 */
	bool operator!() const;

	/**
	 * () operator
	 * -----------
	 *   Build map with input path. Implementation of readDirectory().
	 */
	bool operator()(const std::string_view path);
};

using Error = Helper::Error;

/**
 * getLibVersion()
 * ---------------
 *   To get the version information of libpartition_map
 *   library. It is returned as std::string type.
 */
std::string getLibVersion();

using BuildMap = basic_partition_map_builder;
using Map = basic_partition_map_builder;

} // namespace PartitionMap

#define MAP "libpartition_map"

#endif // #ifndef LIBPARTITION_MAP_LIB_HPP
