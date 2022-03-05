add_executable(read_elf read_elf.cpp)
target_include_directories(read_elf PUBLIC "${INCLUDE_DIR}")
target_link_libraries(read_elf PRIVATE LIB_ELF_READER)