#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
for std in 98 11 14 17 20 23; do
    echo "== C++${std} =="
    build="${root}/build/${std}"
    cmake -S "$root" -B "$build" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_STANDARD="${std}" \
        -DCMAKE_CXX_STANDARD_REQUIRED=ON \
        -DCMAKE_CXX_EXTENSIONS=OFF
    cmake --build "$build" --parallel
    ctest --test-dir "$build" --output-on-failure
done
