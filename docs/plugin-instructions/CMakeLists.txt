#
# Copyright (C) 2026 Yağız Zengin
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

# This definition is only for CLion's code scanning capabilities. When defining plugins, add_plugin() should be used!
add_library(dummy_plugins SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/AdvancedExamplePlugin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ExamplePlugin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AsyncExamplePlugin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/SimpleExamplePlugin.cpp
)
target_link_libraries(dummy_plugins PRIVATE pmt::interface::nolibs)

# Not to be included in the general code.
set_target_properties(dummy_plugins PROPERTIES EXCLUDE_FROM_ALL TRUE)
