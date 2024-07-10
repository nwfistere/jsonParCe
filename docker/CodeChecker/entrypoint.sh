#!/bin/bash

BUILD_DIR=./build
NUM_JOBS=$(grep 'cpu cores' /proc/cpuinfo | uniq | cut -d : -f 2 | xargs)

cmake -S . -B ${BUILD_DIR} -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON
cmake --build ${BUILD_DIR} -j ${NUM_JOBS}
cp ${BUILD_DIR}/compile_commands.json .
CodeChecker analyze ./compile_commands.json --enable-all --output ./reports --jobs ${NUM_JOBS}
CodeChecker parse ./reports
CodeChecker parse --export html --output ./reports_html ./reports