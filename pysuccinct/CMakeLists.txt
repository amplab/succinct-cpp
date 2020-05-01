PYTHON_ADD_MODULE(pycompress pycompress.cpp)
FILE(COPY pycompress.py DESTINATION .)

PYTHON_ADD_MODULE(pyquery_file pyquery_file.cpp)
FILE(COPY pyquery_file.py DESTINATION .)

PYTHON_ADD_MODULE(pyquery_kv pyquery_kv.cpp)
FILE(COPY pyquery_kv.py DESTINATION .)

cmake_minimum_required(VERSION 2.8)
project(succinct-pysuccinct CXX)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    if(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    endif()
endif()
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(INCLUDE include ../core/include)
include_directories(${INCLUDE})

target_link_libraries(pycompress succinct)
target_link_libraries(pyquery_file succinct)
target_link_libraries(pyquery_kv succinct)