#!/bin/bash

OPTIND=1         # Reset in case getopts has been used previously in the shell.
BUILD_DIR=./build/ubuntu
strict_mode="ON"
CC=/usr/bin/gcc
CXX=/usr/bin/g++
run_tests=1
cmake_config="Release"
install_dir=""
package=0

while getopts "h?:s:c:x:t:f:i:p:" opt; do
  case "$opt" in
    h|\?)
      echo "Usage: entrypoint.sh [-s strict_mode] [-c CC] [-x CXX] [-t run_tests] [-f cmake_config] [-i install_dir] [-p package]"
      exit 0 ;;
    s) strict_mode="$OPTARG" ;;
    c) CC="$OPTARG";;
    x) CXX="$OPTARG";;
    t) run_tests="$OPTARG";;
    f) cmake_config="$OPTARG";;
    i) install_dir="$OPTARG";;
    p) package="$OPTARG";;
  esac
done
shift $((OPTIND-1))
[ "${1:-}" = "--" ] && shift

NUM_JOBS=$(grep "cpu cores" /proc/cpuinfo | uniq | cut -d : -f 2 | xargs)
BUILD_DIR="${BUILD_DIR}_${strict_mode}_${cmake_config}"
CMAKE_OPTIONS="-DCMAKE_BUILD_TYPE:STRING=${cmake_config} \
-DJSON_PARCE_ENABLE_TEST:BOOL=ON \
-DJSON_PARCE_ENABLE_EXAMPLE_COMPILE:BOOL=ON \
-DJSON_PARCE_ENABLE_STRICT_MODE:BOOL=${strict_mode}"


echo "BUILD_DIR: ${BUILD_DIR}";
echo "strict_mode: ${strict_mode}";
echo "CC: ${CC}";
echo "CXX: ${CXX}";
echo "run_tests: ${run_tests}";
echo "config: ${cmake_config}";
echo "CMAKE_OPTIONS: ${CMAKE_OPTIONS}";
echo "NUM_JOBS: ${NUM_JOBS}";
echo "install_dir: ${install_dir}";
echo "package: ${package}";

export CC;
export CXX;

rm -rf "${BUILD_DIR}"
cmake -S . -B "${BUILD_DIR}" ${CMAKE_OPTIONS}
cmake --build "${BUILD_DIR}" -j ${NUM_JOBS}

if [[ ${run_tests} -ne 0 ]]; then
  cd "${BUILD_DIR}"
  ctest --output-on-failure -C ${cmake_config} -j ${NUM_JOBS}
  cd -
fi

if [[ -n "${install_dir}" ]]; then
  cmake --install ${BUILD_DIR} --prefix ${install_dir} --config ${cmake_config}
fi

if [[ ${package} -ne 0 ]]; then
  cd "${BUILD_DIR}"
  cpack -C ${cmake_config} -G TGZ
  cd -
fi