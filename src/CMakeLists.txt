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
option(WIND_BUILD_TESTLIB "Build the API for python testing" ON)

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
    COMMAND cmake -S asmjit -B asmjit/build -G "Unix Makefiles" -DASMJIT_STATIC=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    WORKING_DIRECTORY ${EXTERN_DIR}
  )
  execute_process(
    COMMAND make
    WORKING_DIRECTORY ${EXTERN_DIR}/asmjit/build
  )
endif()

include_directories(${ASMJIT_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/core/includes)

file(GLOB_RECURSE SOURCES "core/*.cpp")

add_library(windlib STATIC ${SOURCES})

target_link_libraries(windlib ${ASMJIT_LIB_DIR}/libasmjit.a)
target_compile_options(windlib PRIVATE -O3)
target_include_directories(windlib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/core/includes)
target_compile_definitions(windlib PRIVATE 
    WIND_STD_PATH="${WIND_STD_PATH}"
    WIND_RUNTIME_PATH="${WIND_RUNTIME_PATH}"
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

  target_link_libraries(windtest_api ${ASMJIT_LIB_DIR}/libasmjit.a)

  target_include_directories(windtest_api PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/core/includes
  )

  target_compile_definitions(windtest_api PRIVATE 
    WIND_STD_PATH="${WIND_STD_PATH}"
    WIND_RUNTIME_PATH="${WIND_RUNTIME_PATH}"
  )
endif()