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

include(CMakeParseArguments)
include(${CMAKE_SOURCE_DIR}/build/cmake/interface.cmake)

# Usage: add_super_library(<name> <NO_INTERFACE | USE_INTERFACE | USE_INTERFACE_NOLIBS | USE_INTERFACE_NOLIBS_NOFLAGS> <source(s)> [LIBS <libs> MANUAL_LIBS <libs> MANUAL_LIBS_OF_SHARED <libs> MANUAL_LIBS_OF_STATIC <libs> INCLUDES <includes> COMPILE_OPTIONS <options> LINKER_OPTIONS <linker-options> DEFINATIONS <definations>])
function(add_super_library TARGET_NAME)
    set(options NOPREFIX NO_INTERFACE USE_INTERFACE USE_INTERFACE_NOLIBS USE_INTERFACE_NOLIBS_NOFLAGS SUPER_LIBS)
    set(multiValueArgs SOURCES LIBS MANUAL_LIBS MANUAL_LIBS_OF_SHARED MANUAL_LIBS_OF_STATIC INCLUDES COMPILE_OPTIONS LINKER_OPTIONS DEFINATIONS)
    cmake_parse_arguments(ARG "${options}" "" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "Please specify sources of '${TARGET_NAME}'")
    endif()

    if(ARG_NOPREFIX)
        set(STATIC_TARGET ${TARGET_NAME}_static)
        set(SHARED_TARGET ${TARGET_NAME}_shared)
        set(PREFIX "")
    else()
        set(STATIC_TARGET lib${TARGET_NAME}_static)
        set(SHARED_TARGET lib${TARGET_NAME}_shared)
        set(PREFIX "lib")
    endif()

    add_library(${STATIC_TARGET} STATIC ${ARG_SOURCES})
    add_library(${SHARED_TARGET} SHARED ${ARG_SOURCES})

    foreach(LIB_TARGET ${STATIC_TARGET} ${SHARED_TARGET})
        set_target_properties(${LIB_TARGET} PROPERTIES OUTPUT_NAME ${TARGET_NAME})
        set_target_properties(${LIB_TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        if(ARG_NOPREFIX)
            set_target_properties(${LIB_TARGET} PROPERTIES PREFIX "")
        endif()

        if(ARG_INCLUDES)
            target_include_directories(${LIB_TARGET} PRIVATE ${ARG_INCLUDES})
        endif()
        if(ARG_COMPILE_OPTIONS)
            target_compile_options(${LIB_TARGET} PRIVATE ${ARG_COMPILE_OPTIONS})
        endif()
        if(ARG_LINKER_OPTIONS)
            target_link_options(${LIB_TARGET} PRIVATE ${ARG_LINKER_OPTIONS})
        endif()
        if(ARG_DEFINATIONS)
            target_compile_definitions(${LIB_TARGET} PRIVATE ${ARG_DEFINATIONS})
        endif()
    endforeach()

    if(ARG_USE_INTERFACE)
        target_link_libraries(${STATIC_TARGET} PRIVATE pmt::interface::static)
        target_link_libraries(${SHARED_TARGET} PRIVATE pmt::interface::shared)
    elseif(ARG_USE_INTERFACE_NOLIBS)
        target_link_libraries(${STATIC_TARGET} PRIVATE pmt::interface::nolibs)
        target_link_libraries(${SHARED_TARGET} PRIVATE pmt::interface::nolibs)
    elseif(ARG_USE_INTERFACE_NOLIBS_NOFLAGS)
        target_link_libraries(${STATIC_TARGET} PRIVATE pmt::interface::nolibs_and_flags)
        target_link_libraries(${SHARED_TARGET} PRIVATE pmt::interface::nolibs_and_flags)
    endif()

    if(ARG_LIBS)
        foreach(LIB ${ARG_LIBS})
            if(TARGET "lib${LIB}_static" OR TARGET "${LIB}_static" OR
                    TARGET "lib${LIB}_shared" OR TARGET "${LIB}_shared")

                if(TARGET "lib${LIB}_static")
                    target_link_libraries(${STATIC_TARGET} PRIVATE lib${LIB}_static)
                else()
                    target_link_libraries(${STATIC_TARGET} PRIVATE ${LIB}_static)
                endif()

                if(TARGET "lib${LIB}_shared")
                    target_link_libraries(${SHARED_TARGET} PRIVATE lib${LIB}_shared)
                else()
                    target_link_libraries(${SHARED_TARGET} PRIVATE ${LIB}_shared)
                endif()

            else()
                target_link_libraries(${STATIC_TARGET} PRIVATE ${LIB})
                target_link_libraries(${SHARED_TARGET} PRIVATE ${LIB})
            endif()
        endforeach()
    endif()

    if(ARG_MANUAL_LIBS)
        target_link_libraries(${STATIC_TARGET} PRIVATE ${ARG_MANUAL_LIBS})
        target_link_libraries(${SHARED_TARGET} PRIVATE ${ARG_MANUAL_LIBS})
    endif()

    if(ARG_MANUAL_LIBS_OF_SHARED)
        target_link_libraries(${SHARED_TARGET} PRIVATE ${ARG_MANUAL_LIBS_OF_SHARED})
    endif()
    if(ARG_MANUAL_LIBS_OF_STATIC)
        target_link_libraries(${STATIC_TARGET} PRIVATE ${ARG_MANUAL_LIBS_OF_STATIC})
    endif()
endfunction()

function(add_plugin PLUGIN_NAME)
    set(multiValueArgs SOURCES LIBS COMPILE_OPTIONS LINKER_OPTIONS DEFINATIONS)
    cmake_parse_arguments(ARG "" "" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "Please specify sources of '${PLUGIN_NAME}' plugin")
    endif()

    if(BUILTIN_PLUGINS)
        target_sources(pmt PRIVATE ${ARG_SOURCES})
        target_sources(pmt_static PRIVATE ${ARG_SOURCES})
    else()
        target_sources(pmt_static PRIVATE ${ARG_SOURCES})

        set(TARGET_NAME ${PLUGIN_NAME}_plugin)
        add_library(${TARGET_NAME} SHARED ${ARG_SOURCES})
        set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "" OUTPUT_NAME ${TARGET_NAME})
        target_link_libraries(${TARGET_NAME} PRIVATE pmt::interface::shared)

        if(ARG_LIBS)
            target_link_libraries(${TARGET_NAME} PRIVATE ${ARG_LIBS})
        endif()
        if(ARG_COMPILE_OPTIONS)
            target_compile_options(${TARGET_NAME} PRIVATE ${ARG_COMPILE_OPTIONS})
        endif()
        if(ARG_LINKER_OPTIONS)
            target_link_options(${TARGET_NAME} PRIVATE ${ARG_LINKER_OPTIONS})
        endif()
        if(ARG_DEFINATIONS)
            target_compile_definitions(${TARGET_NAME} PRIVATE ${ARG_DEFINATIONS})
        endif()
    endif()
endfunction()
