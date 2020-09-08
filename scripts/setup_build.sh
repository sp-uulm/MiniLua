#!/usr/bin/env bash
set -ev

mkdir -pv build
pushd build
cmake .. $@
popd

