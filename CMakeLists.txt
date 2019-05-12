cmake_minimum_required(VERSION 3.0)

set(CMAKE_BUILD TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(STATIC_LIBRARY "curl")
project(ssh-reverse-copy-id)

include_directories(include)

file(GLOB SOURCES "src/*.cc")

add_executable(
  ssh-reverse-copy-id
  ${SOURCES}
)

find_library(LIB_LOCATION ${STATIC_LIBRARY})
target_link_libraries(ssh-reverse-copy-id ${LIB_LOCATION})
