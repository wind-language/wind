cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
set(EXTERN_DIR ${PARENT_DIR}/external)

set(WIND_STD_PATH "${CMAKE_CURRENT_LIST_DIR}/std" CACHE STRING "Path to the standard lib files")
set(WIND_RUNTIME_PATH "${CMAKE_CURRENT_LIST_DIR}/runtime" CACHE STRING "Path to the runtime files")
set (WIND_PKGS_PATH "${CMAKE_CURRENT_LIST_DIR}/pkgs" CACHE STRING "Path to the packages directory")
option(WIND_BUILD_TESTLIB "Build the API for python testing" OFF)

message (STATUS "Standard library path: ${WIND_STD_PATH}")
message (STATUS "Runtime path: ${WIND_RUNTIME_PATH}")
message (STATUS "Packages path: ${WIND_PKGS_PATH}")

file(GLOB_RECURSE SOURCES "core/*.cpp")

add_library(windlib STATIC ${SOURCES})

target_compile_options(windlib PRIVATE -O3)
target_include_directories(windlib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/core/includes)
target_compile_definitions(windlib PRIVATE 
    WIND_STD_PATH="${WIND_STD_PATH}"
    WIND_RUNTIME_PATH="${WIND_RUNTIME_PATH}"
    WIND_PKGS_PATH="${WIND_PKGS_PATH}"
  )

if (WIND_BUILD_TESTLIB)
  find_package (Python3 COMPONENTS Interpreter Development)
  if (NOT Python3_FOUND)
    message(FATAL_ERROR "Python3 not found")
  endif()

  execute_process(
    COMMAND python3 -c "import sys; print(str(sys.version_info.major)+'.'+str(sys.version_info.minor))"
    OUTPUT_VARIABLE PYVER
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # Get the extension suffix (e.g., .cpython-<version>-<arch>.so)
  execute_process(
    COMMAND python3-config --extension-suffix
    OUTPUT_VARIABLE PYSUFFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REPLACE ".so" "" PYSUFFIX ${PYSUFFIX})

  execute_process(
    COMMAND python3 -m pybind11 --include
    OUTPUT_VARIABLE PYBIND11_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  message(STATUS "Python include directories: ${PYBIND11_INCLUDE_DIR}")
  message(STATUS "Python version: ${PYVER}")

  add_library(windtest_api SHARED ${SOURCES} ${CMAKE_CURRENT_LIST_DIR}/pyapi/api.cpp)

  set_target_properties(windtest_api PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${TESTLIB_OUTDIR}
    LIBRARY_OUTPUT_NAME pyapi${PYSUFFIX}
  )

  target_compile_options(windtest_api PUBLIC -O3 -fPIC -std=c++17 -I/usr/include/python${PYVER} -I/usr/lib/python${PYVER}/site-packages/pybind11/include)

  target_include_directories(windtest_api PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/core/includes
  )

  target_compile_definitions(windtest_api PRIVATE 
    WIND_STD_PATH="${WIND_STD_PATH}"
    WIND_RUNTIME_PATH="${WIND_RUNTIME_PATH}"
    WIND_PKGS_PATH="${WIND_PKGS_PATH}"
  )
endif()