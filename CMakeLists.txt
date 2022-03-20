cmake_minimum_required(VERSION 3.16)
project(C_BDD DESCRIPTION "Byjtew Dumb Debugger" LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Adding /extern to fetch each of its .cmake
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/extern ${CMAKE_MODULE_PATH})

# CMake known libraries:
find_package(TBB REQUIRED)
find_package(Threads REQUIRED)

# Libunwind:
find_package(Libunwind REQUIRED)
find_library(LIBUNWIND_PTRACE_LIBRARIES NAMES unwind-ptrace)
set(LIBUNWIND_LIBRARIES ${LIBUNWIND_LIBRARIES};${LIBUNWIND_PTRACE_LIBRARIES})
find_library(LIBUNWIND_GENERIC_LIBRARIES NAMES unwind-generic)
set(LIBUNWIND_LIBRARIES ${LIBUNWIND_LIBRARIES};${LIBUNWIND_GENERIC_LIBRARIES})

# Headers:
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
add_subdirectory(lib)

# Sample programs to debug:
add_subdirectory(samples)

# Executables:
add_subdirectory(apps)