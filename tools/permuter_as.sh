#!/usr/bin/env bash
set -euo pipefail

input=""
output=""
args=("$@")
i=0
while [[ $i -lt ${#args[@]} ]]; do
    arg="${args[$i]}"
    if [[ "$arg" == "-o" ]]; then
        output="${args[$((i + 1))]}"
        i=$((i + 2))
    elif [[ "$arg" == -* ]]; then
        i=$((i + 1))
    else
        input="$arg"
        i=$((i + 1))
    fi
done

if [[ -z "$input" || -z "$output" ]]; then
    echo "usage: $0 input.s -o output.o" >&2
    exit 2
fi

mipsel-unknown-linux-gnu-as -march=vr4300 -mabi=32 -G8 -Iinclude "$input" -o "$output"
