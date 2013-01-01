cmake_minimum_required(VERSION 2.6)
project(kernelat)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-scripts)
find_package(ZMQ REQUIRED)
include_directories(${ZMQ_INCLUDE_DIRS})
find_package(PWW REQUIRED)
include_directories(${PWW_INCLUDE_DIRS})

add_definitions(-O3 -std=c99 -W -Wall -pedantic -D_GNU_SOURCE)

add_subdirectory(kernelat-spawner)
add_subdirectory(kernelat-child)
