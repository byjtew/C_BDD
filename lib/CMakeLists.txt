add_library(LIB_ELF_READER STATIC elf_reader.cpp ${INCLUDE_DIR}/elf_reader.hpp)
set_target_properties(LIB_ELF_READER PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
target_include_directories(LIB_ELF_READER PUBLIC ${INCLUDE_DIR})

add_library(LIB_TRACING STATIC tracing.cpp ${INCLUDE_DIR}/tracing.hpp elf_reader.cpp ${INCLUDE_DIR}/elf_reader.hpp)
set_target_properties(LIB_TRACING PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
target_include_directories(LIB_TRACING PUBLIC ${INCLUDE_DIR})
target_link_libraries(LIB_TRACING PUBLIC LIB_ELF_READER)