#!/usr/bin/env bash

export MAKEFLAGS=-j4

# Install dependencies using Nix, if installed.
if command -v nix >/dev/null && [ -n "IN_MINILUA_NIX_SHELL" ]
then
    . <(nix print-dev-env -f "$(dirname "${BASH_SOURCE[0]}")/../shell.nix")
fi
