#!/usr/bin/env bash
set -ev

source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

pushd build
bash <(curl -s https://codecov.io/bash) -f coverage.info.cleaned
popd
