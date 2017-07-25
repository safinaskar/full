cmake_minimum_required (VERSION 3.1) # 3.1 adds C_STANDARD
project (full LANGUAGES CXX)
add_executable (full full.cpp)
set_property (TARGET full PROPERTY CXX_STANDARD 14)
target_link_libraries (full -lsh)
install (TARGETS full DESTINATION bin)
