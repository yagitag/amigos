include_directories(${Search_SOURCE_DIR}/${INC_DIR}/indexer/)
include_directories(${Search_SOURCE_DIR}/${CONTRIB_DIR}/tinyxml2/)
include_directories(${Search_SOURCE_DIR}/${CONTRIB_DIR}/murmur_hash/)
include_directories(${Indexer_SOURCE_DIR}/${INC_DIR}/)

add_library(index index_struct.cpp)
add_executable(indexer indexer.cpp memory_control.cpp)

find_package(Boost COMPONENTS system filesystem REQUIRED)
if (NOT Boost_FOUND)
  message(SEND_ERROR "Failed to find boost libraries.")
  return()
else()
  include_directories(${Boost_INCLUDE_DIRS})
endif()

target_link_libraries(indexer index murmur_hash tinyxml2 common ${Boost_LIBRARIES})
