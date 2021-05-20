#!/usr/bin/env bash
set -ex

source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

pushd build
make MiniLua-newgui
popd

time ./build/examples/MiniluaGui/MiniLua-newgui "$@"

