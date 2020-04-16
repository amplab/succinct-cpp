cmake_minimum_required(VERSION 2.8)
project(succinct CXX)

FIND_PACKAGE(PythonInterp "3.7")

if (PYTHONINTERP_FOUND)
  if (UNIX AND NOT APPLE)
    if (PYTHON_VERSION_MAJOR EQUAL 3)
        FIND_PACKAGE(Boost COMPONENTS python${PYTHON_VERSION_SUFFIX})
        FIND_PACKAGE(PythonInterp 3)
        FIND_PACKAGE(PythonLibs 3 REQUIRED)
    else()
        FIND_PACKAGE(Boost COMPONENTS python)
        FIND_PACKAGE(PythonInterp)
        FIND_PACKAGE(PythonLibs REQUIRED)
    endif()
  else()	
    if (PYTHON_VERSION_MAJOR EQUAL 3)
        FIND_PACKAGE(Boost COMPONENTS python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR})
        FIND_PACKAGE(PythonInterp 3)
        FIND_PACKAGE(PythonLibs 3 REQUIRED)
    else()
        FIND_PACKAGE(Boost COMPONENTS python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR})
        FIND_PACKAGE(PythonInterp)
        FIND_PACKAGE(PythonLibs REQUIRED)
    endif()
  endif()
else()
    message("Python not found")
endif()

message(STATUS "PYTHON_LIBRARIES = ${PYTHON_LIBRARIES}")
message(STATUS "PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
message(STATUS "PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES = ${Boost_LIBRARIES}")

ENABLE_TESTING()
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
LINK_LIBRARIES(${Boost_LIBRARIES} ${PYTHON_LIBRARIES}) # Deprecated but so convenient!

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
enable_language(C)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-modules)

# Builds all of Succinct's sub-modules
add_subdirectory(core)
add_subdirectory(examples)
add_subdirectory(sharded)
add_subdirectory(sharded-kv)
add_subdirectory(bench)
add_subdirectory(pysuccinct)

# Testing framework
# Build google testing framework
add_subdirectory(external/gtest-1.7.0)

# Build and run all tests for succinct
add_subdirectory(test)

enable_testing()
add_test(CoreTest ${TESTS_PATH}/core_test ${PROJECT_SOURCE_DIR}/test/data)
