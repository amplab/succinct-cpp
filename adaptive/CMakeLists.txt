cmake_minimum_required(VERSION 2.8)
project(succinct-adaptive CXX)

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

set(GENERAL_SOURCES ${SOURCES_DIR}/adaptive_constants.cpp
    ${SOURCES_DIR}/adaptive_types.cpp)

set(SERVER_SOURCES ${GENERAL_SOURCES}
    ${SOURCES_DIR}/AdaptiveQueryService.cpp
    ${SOURCES_DIR}/AdaptiveQueryServer.cpp)

set(HANDLER_SOURCES ${GENERAL_SOURCES}
    ${SOURCES_DIR}/AdaptiveQueryService.cpp
    ${SOURCES_DIR}/AdaptiveSuccinctService.cpp
    ${SOURCES_DIR}/AdaptiveSuccinctServer.cpp)

set(MASTER_SOURCES ${GENERAL_SOURCES}
    ${SOURCES_DIR}/AdaptiveSuccinctService.cpp
    ${SOURCES_DIR}/AdaptiveMasterService.cpp
    ${SOURCES_DIR}/AdaptiveSuccinctMaster.cpp)

set(CLIENT_SOURCES ${GENERAL_SOURCES}
    ${SOURCES_DIR}/AdaptiveQueryService.cpp
    ${SOURCES_DIR}/AdaptiveSuccinctService.cpp
    ${SOURCES_DIR}/AdaptiveMasterService.cpp)

include_directories(${INCLUDE_DIR} ${CORE_INCLUDE_DIR})

add_executable(aserver ${SERVER_SOURCES})
add_executable(ahandler ${HANDLER_SOURCES})
add_executable(amaster ${MASTER_SOURCES})
add_library(aclient ${CLIENT_SOURCES})

target_link_libraries(aserver succinct)
target_link_libraries(aserver ${THRIFT_LIBRARIES})
target_link_libraries(ahandler ${THRIFT_LIBRARIES})
target_link_libraries(amaster ${THRIFT_LIBRARIES})
