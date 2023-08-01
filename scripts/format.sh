#!/usr/bin/env bash

DIR=$(dirname "${BASH_SOURCE[0]}")
source "$DIR/_common.sh"

"${CLANG_FORMAT:-clang-format}" -i $FILES "$@"