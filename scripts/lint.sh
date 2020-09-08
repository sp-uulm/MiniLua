#!/usr/bin/env bash
set -ev

DIR=$(dirname "${BASH_SOURCE[0]}")
source "$DIR/_common.sh"

clang-tidy $FILES $@
