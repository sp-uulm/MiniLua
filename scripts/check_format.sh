#!/usr/bin/env bash
set -evx

CLANG_FORMAT=${CLANG_FORMAT:-clang-format}

DIR=$(dirname "${BASH_SOURCE[0]}")
source "$DIR/_common.sh"

exec "$CLANG_FORMAT" --style=file --dry-run -Werror $FILES
