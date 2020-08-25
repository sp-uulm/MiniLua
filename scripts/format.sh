#!/usr/bin/env bash

DIR=$(dirname "${BASH_SOURCE[0]}")
source "$DIR/_common.sh"

clang-format $FILES $@
