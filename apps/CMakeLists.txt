add_executable(readelf readelf.cpp)
target_include_directories(readelf PUBLIC "${INCLUDE_DIR}")
target_link_libraries(readelf PRIVATE LIB_ELF_READER)

find_package(Threads REQUIRED)
add_executable(tracer tracer.cpp)
target_include_directories(tracer PUBLIC "${INCLUDE_DIR}")
target_link_libraries(tracer PRIVATE LIB_TRACING TBB::tbb Threads::Threads)


#[[
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lunwind -lunwind-ptrace -lunwind-generic")

add_executable(unwind_pid unwind_pid.c)
target_include_directories(unwind_pid PUBLIC "${INCLUDE_DIR}")
target_compile_options(unwind_pid PRIVATE -lunwind -lunwind-ptrace -lunwind-generic)
]]

