cmake_minimum_required(VERSION 2.8)
project(succinct-bench CXX)

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")

find_package(Thrift REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(INCLUDE include ../core/include ../sharded/include)
include_directories(${INCLUDE})
add_executable(fbench src/file_benchmark.cc)
add_executable(sbench src/shard_benchmark.cc)
add_executable(surebench src/regex_benchmark.cc)
add_executable(ssbench src/succinct_benchmark.cc)

target_link_libraries(fbench succinct)
target_link_libraries(sbench succinct)
target_link_libraries(surebench succinct)
target_link_libraries(ssbench sclient ${THRIFT_LIBRARIES})
