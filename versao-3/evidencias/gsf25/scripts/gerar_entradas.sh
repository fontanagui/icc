#!/usr/bin/env bash
# Gera casos no formato N x0 epsilon MAX; execute cada linha separadamente.
set -euo pipefail
if (( $# == 0 )); then
    set -- 32 64 128 256 512 1000 2000 4000 8000 9000 10000 20000
fi
for n in "$@"; do
    printf '%s -1 0 25\n' "$n"
done
