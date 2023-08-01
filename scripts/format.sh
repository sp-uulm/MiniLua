#!/usr/bin/env nix-shell
#!nix-shell -i bash --pure ../shell.nix

DIR=$(dirname "${BASH_SOURCE[0]}")
source "$DIR/_common.sh"

"${CLANG_FORMAT:-clang-format}" -i $FILES "$@"