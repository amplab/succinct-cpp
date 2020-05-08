cmake_minimum_required(VERSION 2.8)
project(succinct-pysuccinct CXX)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(INCLUDE include ../core/include)
include_directories(${INCLUDE})