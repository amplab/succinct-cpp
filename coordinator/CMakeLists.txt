cmake_minimum_required(VERSION 2.8)
project(succinct-coordinator CXX)

find_package(Thrift REQUIRED)
find_package(Threads REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(UTILS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../utils/include)

file(GLOB_RECURSE COORDINATOR_SOURCES ${SOURCES_DIR}/*.cc)

include_directories(${INCLUDE_DIR} ${SERVER_INCLUDE_DIR} ${UTILS_INCLUDE_DIR})

add_executable(scoordinator ${COORDINATOR_SOURCES})

target_link_libraries(scoordinator sutils ${THRIFT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
