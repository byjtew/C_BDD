add_library(BDD_exclusive_io STATIC bdd_exclusive_io.cpp ${INCLUDE_DIR}/bdd_exclusive_io.hpp)
set_target_properties(BDD_exclusive_io PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
target_include_directories(BDD_exclusive_io PUBLIC ${INCLUDE_DIR})
target_link_libraries(BDD_exclusive_io PUBLIC TBB::tbb Threads::Threads)


add_library(BDD_elf STATIC bdd_elf.cpp ${INCLUDE_DIR}/bdd_elf.hpp)
set_target_properties(BDD_elf PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
target_include_directories(BDD_elf PUBLIC ${INCLUDE_DIR})


add_library(BDD_ptrace STATIC bdd_ptrace_breakpoint.cpp bdd_ptrace.cpp ${INCLUDE_DIR}/bdd_ptrace.hpp)
set_target_properties(BDD_ptrace PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
target_include_directories(BDD_ptrace PUBLIC ${INCLUDE_DIR})
target_link_libraries(BDD_ptrace PUBLIC BDD_elf BDD_exclusive_io)
