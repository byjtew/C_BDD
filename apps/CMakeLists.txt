add_executable(readelf readelf.cpp)
target_include_directories(readelf PUBLIC "${INCLUDE_DIR}")
target_link_libraries(readelf PRIVATE BDD_elf BDD_exclusive_io)

add_executable(c_bdd tracer.cpp)
target_include_directories(c_bdd PUBLIC "${INCLUDE_DIR}")
target_link_libraries(c_bdd PRIVATE BDD_elf BDD_exclusive_io BDD_ptrace)
