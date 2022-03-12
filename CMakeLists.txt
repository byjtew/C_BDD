cmake_minimum_required(VERSION 3.16)
project(C_BDD DESCRIPTION "Byjtew Dumb Debugger" LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(TBB REQUIRED)

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
add_subdirectory(lib)

add_subdirectory(samples)

add_subdirectory(apps)