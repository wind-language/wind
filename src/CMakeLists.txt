cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
set(EXTERN_DIR ${PARENT_DIR}/external)

add_definitions(-DASMJIT_STATIC)

set(ASMJIT_INCLUDE_DIR ${EXTERN_DIR}/asmjit/src)
set(ASMJIT_LIB_DIR ${EXTERN_DIR}/asmjit/build)

set(WIND_STD_PATH "${CMAKE_CURRENT_LIST_DIR}/std" CACHE STRING "Path to the standard lib files")
set(WIND_RUNTIME_PATH "${CMAKE_CURRENT_LIST_DIR}/runtime" CACHE STRING "Path to the runtime files")

# ugly asmjit build process, I don't want to use external project
# it's not necessary to build asmjit every time
# as bug fixes are made on rarely used instructions and on aarch64 architecture
# so it's safe to assume that the library shouldn't encounter any bugs

if(NOT EXISTS "${ASMJIT_LIB_DIR}/libasmjit.a")
  if (NOT EXISTS ${EXTERN_DIR})
    file(MAKE_DIRECTORY ${EXTERN_DIR})
  endif()
  execute_process(
    COMMAND git clone https://github.com/asmjit/asmjit.git asmjit
    WORKING_DIRECTORY ${EXTERN_DIR}
  )
  execute_process(
    COMMAND cmake -S asmjit -B asmjit/build -G "Unix Makefiles" -DASMJIT_STATIC=ON
    WORKING_DIRECTORY ${EXTERN_DIR}
  )
  execute_process(
    COMMAND make
    WORKING_DIRECTORY ${EXTERN_DIR}/asmjit/build
  )
endif()

include_directories(${ASMJIT_INCLUDE_DIR})
message(${ASMJIT_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/core/includes)

file(GLOB_RECURSE SOURCES "core/*.cpp")

add_library(windlib STATIC ${SOURCES})

target_link_libraries(windlib ${ASMJIT_LIB_DIR}/libasmjit.a)
target_compile_options(windlib PRIVATE -O3)
target_include_directories(windlib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/core/includes)
target_compile_definitions(windlib PRIVATE WIND_STD_PATH="${WIND_STD_PATH}")
target_compile_definitions(windlib PRIVATE WIND_RUNTIME_PATH="${WIND_RUNTIME_PATH}")