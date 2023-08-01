#!/usr/bin/env bash

source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

file="$1"

cmd="build/bin/MiniLua-bin --quiet"
tmpout=/tmp/lua_tmpout.txt
tmperr=/tmp/lua_tmperr.txt

dirname=$(dirname "$file")
basename=$(basename "$file" .lua)
stdin_file="$dirname/$basename.in"
stdout_file="$dirname/$basename.out"
stderr_file="$dirname/$basename.err"

diff() {
    git -c core.pager= diff --no-index "$@"
}

if [[ -f $stdin_file ]]; then
    cat "$stdin_file" | $cmd "$file" >$tmpout 2>$tmperr
else
    $cmd "$file" >$tmpout 2>$tmperr
fi

if [[ -f $stdout_file ]]; then
    echo "== stdout diff"
    diff "$stdout_file" "$tmpout" || exit 1
elif [[ -s $tmpout ]]; then
    echo "== extected no stdout but got"
    cat $tmpout
    exit 1
fi

if [[ -f $stderr_file ]]; then
    echo "== stderr diff"
    diff "$stderr_file" "$tmperr" || exit 1
elif [[ -s $tmperr ]]; then
    echo "== extected no stderr but got"
    cat $tmperr
    exit 1
fi
