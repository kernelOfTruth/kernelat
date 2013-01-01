cmake_minimum_required(VERSION 2.6)
project(kernelat)

add_definitions(-O3 -std=c99 -W -Wall -pedantic -D_GNU_SOURCE)

add_subdirectory(kernelat-spawner)
add_subdirectory(kernelat-child)
