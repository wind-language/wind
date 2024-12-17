cmake_minimum_required(VERSION 3.10)

project(Wind VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(src)

set(SRC_FILES src/main/proc.cpp)

add_executable(wind ${SRC_FILES})
target_link_libraries(wind windlib)
