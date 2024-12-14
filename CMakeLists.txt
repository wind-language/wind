cmake_minimum_required(VERSION 3.10)

project(wind VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_definitions(-DASMJIT_STATIC)

set(ASMJIT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/asmjit/src")
set(ASMJIT_LIB_DIR "${CMAKE_SOURCE_DIR}/asmjit/build")

include_directories(${ASMJIT_INCLUDE_DIR})
include_directories("src/includes/")

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(wind ${SOURCES})

if(NOT EXISTS "${ASMJIT_LIB_DIR}/build/libasmjit.a")
  execute_process(
    COMMAND git clone https://github.com/asmjit/asmjit.git asmjit
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  execute_process(
    COMMAND cmake -S asmjit -B asmjit/build -G "Unix Makefiles" -DASMJIT_STATIC=ON
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  execute_process(
    COMMAND make
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/asmjit/build
  )
endif()

target_link_libraries(wind ${ASMJIT_LIB_DIR}/libasmjit.a)
