#!/usr/bin/env bash
set -ev

source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

mkdir -pv build
pushd build

rm -f CMakeCache.txt
rm -rf CMakeFiles

cmake .. $@
popd

