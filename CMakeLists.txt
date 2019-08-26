cmake_minimum_required(VERSION 3.1)

project(ttga)
set(CMAKE_CXX_STANDARD 11)

option(WITH_KCRASH "KCrash integration" OFF)
option(WITH_IPELIB "Ipe integration - needs Ipelib" OFF)
option(DISABLE_SLOW_ASSERTS "Disable slow asserts in debug mode" OFF)
option(BUILD_TESTS "Build the unit tests" ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5Widgets REQUIRED)

if(WITH_KCRASH)
	find_package(KF5Crash REQUIRED)
	add_definitions(-DWITH_KCRASH)
endif(WITH_KCRASH)

if(WITH_IPELIB)
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
	find_package(Ipelib REQUIRED)
	include_directories(${Ipelib_INCLUDE_DIRS})
	add_definitions(-DWITH_IPELIB)
endif(WITH_IPELIB)

if(DISABLE_SLOW_ASSERTS)
	add_definitions(-DDISABLE_SLOW_ASSERTS)
endif(DISABLE_SLOW_ASSERTS)

add_subdirectory(src)
