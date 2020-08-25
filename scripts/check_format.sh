#!/usr/bin/env bash

DIR=$(dirname "${BASH_SOURCE[0]}")
source "$DIR/_common.sh"

MESSAGES=$(clang-format --style=file --dry-run --color $FILES 2>&1)

if [[ ! -z "$MESSAGES" ]]; then
    echo "$MESSAGES"
    exit 1
fi
