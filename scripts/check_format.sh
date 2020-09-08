#!/usr/bin/env bash
set -ev

DIR=$(dirname "${BASH_SOURCE[0]}")
source "$DIR/_common.sh"

exec clang-format --style=file --dry-run -Werror $FILES
