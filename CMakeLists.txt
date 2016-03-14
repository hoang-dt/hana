cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(hana LANGUAGES CXX)

find_package(Threads)

add_subdirectory(src)
