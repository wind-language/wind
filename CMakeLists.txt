cmake_minimum_required(VERSION 3.10)

project(Wind VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


set(TESTLIB_OUTDIR ${CMAKE_CURRENT_LIST_DIR}/testing/suite/lib)
if (NOT EXISTS ${TESTLIB_OUTDIR})
    file(MAKE_DIRECTORY ${TESTLIB_OUTDIR})
endif()
add_subdirectory(src)

set(SRC_FILES src/main/proc.cpp)

add_executable(wind ${SRC_FILES})
target_link_libraries(wind windlib)
