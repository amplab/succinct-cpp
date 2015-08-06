#!/bin/bash

mkdir -p build
cd build
cmake ../
START=$(date +%s)
make && make test
END=$(date +%s)
echo "Total Build time (real) = $(( $END - $START )) seconds"
