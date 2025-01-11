cmake_minimum_required(VERSION 3.10)

project(Wind VERSION 1.0)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
option(WIND_BUILD_RUNTIME "Build the runtime" ON)


set(TESTLIB_OUTDIR ${CMAKE_CURRENT_LIST_DIR}/testing/suite/lib)
if (NOT EXISTS ${TESTLIB_OUTDIR})
    file(MAKE_DIRECTORY ${TESTLIB_OUTDIR})
endif()
add_subdirectory(src)

set(SRC_FILES src/main/proc.cpp)

add_executable(windc ${SRC_FILES})
target_link_libraries(windc windlib)
target_compile_options(windc PRIVATE -g)

if (WIND_BUILD_RUNTIME)
    add_custom_command(
        TARGET windc POST_BUILD
        COMMAND ${CMAKE_BINARY_DIR}/windc ${WIND_RUNTIME_PATH}/stack.w ${WIND_RUNTIME_PATH}/handler.w ${WIND_RUNTIME_PATH}/start.w -ej -o ${WIND_RUNTIME_PATH}/wrt.o
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMENT "Compiling runtime utilities"
    )
endif()