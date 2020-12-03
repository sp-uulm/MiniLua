#!/usr/bin/env bash
set -ev

mkdir -pv build
pushd build

rm CMakeCache.txt
rm -r CMakeFiles

cmake .. $@
popd

