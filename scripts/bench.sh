#!/usr/bin/env bash
set -ex

pushd build
build_type=$(cmake -L .. | grep CMAKE_BUILD_TYPE | sed 's/CMAKE_BUILD_TYPE:STRING=//g')
popd

if [[ "$build_type" != "Release" ]]; then
    ./scripts/setup_build.sh "-DCMAKE_BUILD_TYPE=Release"
fi

pushd build
make MiniLua-bench
./bench/MiniLua-bench "$@"
popd
