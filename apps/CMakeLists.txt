add_executable(readelf readelf.cpp)
target_include_directories(readelf PUBLIC "${INCLUDE_DIR}")
target_link_libraries(readelf PRIVATE LIB_ELF_READER)