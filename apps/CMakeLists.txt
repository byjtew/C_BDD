add_executable(readelf readelf.cpp)
target_include_directories(readelf PUBLIC "${INCLUDE_DIR}")
target_link_libraries(readelf PRIVATE BDD_elf BDD_exclusive_io)

add_executable(tracer tracer.cpp)
target_include_directories(tracer PUBLIC "${INCLUDE_DIR}")
target_link_libraries(tracer PRIVATE BDD_elf BDD_exclusive_io BDD_ptrace)
