cmake_minimum_required(VERSION 3.15)

project(topotide
	DESCRIPTION "TopoTide"
	HOMEPAGE_URL "https://github.com/tue-alga/ttga"
	LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

option(DISABLE_SLOW_ASSERTS "Disable slow asserts in debug mode" OFF)
option(BUILD_TESTS "Build the unit tests" ON)
option(EXPERIMENTAL_FINGERS_SUPPORT "Include support for detecting fingers (warning: experimental!)" OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Svg)
find_package(GDAL REQUIRED)

if(DISABLE_SLOW_ASSERTS)
	add_definitions(-DDISABLE_SLOW_ASSERTS)
endif(DISABLE_SLOW_ASSERTS)

if(EXPERIMENTAL_FINGERS_SUPPORT)
	add_definitions(-DEXPERIMENTAL_FINGERS_SUPPORT)
endif(EXPERIMENTAL_FINGERS_SUPPORT)

add_subdirectory(lib)
add_subdirectory(gui)
if(BUILD_TESTS)
	add_subdirectory(test)
endif(BUILD_TESTS)
