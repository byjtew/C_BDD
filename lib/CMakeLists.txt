add_library(LIB_ELF_READER elf_reader.cpp)
set_target_properties(LIB_ELF_READER PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
target_include_directories(LIB_ELF_READER PUBLIC "${CMAKE_SOURCE_DIR}/include")