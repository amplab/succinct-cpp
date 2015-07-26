cmake_minimum_required(VERSION 2.8)
project(succinct-test CXX)

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

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
set(TESTS_PATH ${EXECUTABLE_OUTPUT_PATH} PARENT_SCOPE)

set(CORE_INCLUDES ../core/include)
set(THRIFT_INCLUDES ../thrift/include)
set(ADAPTIVE_INCLUDES ../adaptive/include)

include_directories(${gtest_SOURCE_DIR}/include
    ${CORE_INCLUDES}
    ${THRIFT_INCLUDES}
    ${ADAPTIVE_INCLUDES})

set(test_sources src/SuccinctCoreTest.cpp src/test_main.cpp)
add_executable(core_tests ${test_sources})
target_link_libraries(core_tests gtest_main succinct)
