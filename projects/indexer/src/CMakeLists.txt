#include_directories(${Search_SOURCE_DIR}/${INC_DIR}/)
include_directories(${Indexer_SOURCE_DIR}/${INC_DIR}/)

add_executable(indexer indexer.cpp)

find_package(Boost COMPONENTS system filesystem REQUIRED)
if (NOT Boost_FOUND)
  message(SEND_ERROR "Failed to find boost libraries.")
  return()
else()
  include_directories(${Boost_INCLUDE_DIRS})
endif()

target_link_libraries(indexer tinyxml2 common ${Boost_LIBRARIES})
