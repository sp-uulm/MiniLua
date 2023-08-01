#!/usr/bin/env bash

source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

file="$1"

cmd=build/bin/MiniLua-bin

dirname=$(dirname "$file")
basename=$(basename "$file" .lua)
stdout_file="$dirname/$basename.out"
stderr_file="$dirname/$basename.err"

"$cmd" "$file" >"$stdout_file" 2>"$stderr_file"

if [[ ! -s $stdout_file ]]; then
    rm "$stdout_file"
    echo "No stdout"
else
    echo "Updated $stdout_file"
fi

if [[ ! -s $stderr_file ]]; then
    rm "$stderr_file"
    echo "No stderr"
else
    echo "Updated $stderr_file"
fi
