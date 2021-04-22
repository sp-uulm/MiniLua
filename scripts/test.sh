#!/usr/bin/env bash
set -ex

source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

pushd build
make MiniLua-tests
./tests/MiniLua-tests "$@"
popd

./scripts/run_luatests.sh
