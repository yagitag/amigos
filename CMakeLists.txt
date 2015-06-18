cmake_minimum_required(VERSION 2.8)

project(Search)

set(INC_DIR include)
set(CONTRIB_DIR contrib)
set(SRC_DIR src)
set(EXECUTABLE_OUTPUT_PATH ${Search_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include_directories(${INC_DIR}/common/)

add_subdirectory(projects/common)
add_subdirectory(projects/indexer)
add_subdirectory(contrib/tinyxml2)
add_subdirectory(contrib/murmur_hash)
add_subdirectory(contrib/leveldb)
