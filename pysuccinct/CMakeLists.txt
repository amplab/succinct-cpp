cmake_minimum_required(VERSION 2.8)
project(succinct-pysuccinct CXX)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(INCLUDE include ../core/include)
include_directories(${INCLUDE})

python_add_module(file file.cpp)
python_add_module(kvstore kvstore.cpp)
python_add_module(semistructuredstore semistructuredstore.cpp)

target_link_libraries(file succinct)
target_link_libraries(kvstore succinct)
target_link_libraries(semistructuredstore succinct)