cmake_minimum_required(VERSION 2.8)
project(succinct-sharded CXX)

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

find_package(Thrift REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core/include)

set(GENERAL_SOURCES ${SOURCES_DIR}/succinct_constants.cc
    ${SOURCES_DIR}/succinct_types.cc)

set(SERVER_SOURCES ${GENERAL_SOURCES}
    ${SOURCES_DIR}/QueryService.cc
    ${SOURCES_DIR}/query_server.cc)

set(HANDLER_SOURCES ${GENERAL_SOURCES}
    ${SOURCES_DIR}/QueryService.cc
    ${SOURCES_DIR}/AggregatorService.cc
    ${SOURCES_DIR}/aggregator.cc)

set(CLIENT_SOURCES ${GENERAL_SOURCES}
    ${SOURCES_DIR}/QueryService.cc
    ${SOURCES_DIR}/AggregatorService.cc)

include_directories(${INCLUDE_DIR} ${CORE_INCLUDE_DIR} ${THRIFT_INCLUDE_DIR})

add_executable(sserver ${SERVER_SOURCES})
add_executable(saggregator ${HANDLER_SOURCES})
add_library(sclient ${CLIENT_SOURCES})
add_executable(sinitializer ${SOURCES_DIR}/initializer.cc)

target_link_libraries(sserver succinct)
target_link_libraries(sserver ${THRIFT_LIBRARIES})
target_link_libraries(saggregator ${THRIFT_LIBRARIES})
target_link_libraries(sinitializer sclient ${THRIFT_LIBRARIES})
