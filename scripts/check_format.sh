#!/usr/bin/env bash
set -ev

DIR=$(dirname "${BASH_SOURCE[0]}")
source "$DIR/_common.sh"
COLOR=--color

MESSAGES=$(clang-format --style=file --dry-run $COLOR $FILES 2>&1)

if [[ ! -z "$MESSAGES" ]]; then
    echo "$MESSAGES"
    echo
    echo "Code not correctly formatted!"
    exit 1
fi
