#!/usr/bin/env bash
set -ex

source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

pushd build
make MiniLua-newgui2
popd

time ./build/examples/MiniluaGui2/MiniLua-newgui2 "$@"

