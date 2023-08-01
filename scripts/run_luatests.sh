#!/usr/bin/env bash

source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

pushd build
make MiniLua-bin
popd

for file in luaprograms/unit_tests/**/*.lua; do
    echo "== Running test $file"
    if ./scripts/_run_luatest.sh "$file"; then
        echo "== SUCCESS"
    else
        echo "== FAILURE"
        exit 1
    fi
done
