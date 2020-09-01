#!/usr/bin/env bash
set -ex

pushd build
make MiniLua-tests
./tests/MiniLua-tests "$@"
popd

