#!/usr/bin/env bash
set -ev

pushd build
make MiniLua-tests-coverage $@
popd

