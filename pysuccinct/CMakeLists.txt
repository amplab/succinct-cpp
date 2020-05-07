python_add_module(pycompress pycompress.cpp)
file(COPY pycompress.py DESTINATION .)

python_add_module(pyquery_file pyquery_file.cpp)
file(COPY pyquery_file.py DESTINATION .)

python_add_module(pyquery_kv pyquery_kv.cpp)
file(COPY pyquery_kv.py DESTINATION .)

python_add_module(pyquery_semistructured pyquery_semistructured.cpp)
file(COPY pyquery_semistructured.py DESTINATION .)

cmake_minimum_required(VERSION 2.8)
project(succinct-pysuccinct CXX)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(INCLUDE include ../core/include)
include_directories(${INCLUDE})

target_link_libraries(pycompress succinct)
target_link_libraries(pyquery_file succinct)
target_link_libraries(pyquery_kv succinct)
target_link_libraries(pyquery_semistructured succinct)