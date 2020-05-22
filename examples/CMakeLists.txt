cmake_minimum_required(VERSION 2.8)
project(succinct-examples)

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
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -ldl -lutil -I/usr/local/bin/python3.7")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(INCLUDE include ../core/include)
include_directories(${INCLUDE})

add_executable(compress src/compress.cc)
add_executable(query_file src/query_file.cc)
add_executable(query_kv src/query_kv.cc)
add_executable(query_semistructured src/query_semistructured.cc)

file(COPY lambda/lambdatest.py DESTINATION ${EXECUTABLE_OUTPUT_PATH})

file(COPY pysuccinct/pyfile.py DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY pysuccinct/pykv.py DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY pysuccinct/pysemistructured.py DESTINATION ${EXECUTABLE_OUTPUT_PATH})

file(COPY lambda/lambdatest.py DESTINATION ${EXECUTABLE_OUTPUT_PATH})

target_link_libraries(compress succinct)
target_link_libraries(query_file succinct)
target_link_libraries(query_kv succinct)
target_link_libraries(query_semistructured succinct)