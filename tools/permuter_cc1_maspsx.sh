#!/usr/bin/env bash
set -euo pipefail

input=""
output=""
cpp_args=()
args=("$@")
i=0
while [[ $i -lt ${#args[@]} ]]; do
    arg="${args[$i]}"
    case "$arg" in
        -o)
            output="${args[$((i + 1))]}"
            i=$((i + 2))
            ;;
        -I|-D|-U)
            cpp_args+=("$arg" "${args[$((i + 1))]}")
            i=$((i + 2))
            ;;
        -I*|-D*|-U*|-nostdinc)
            cpp_args+=("$arg")
            i=$((i + 1))
            ;;
        *)
            input="$arg"
            i=$((i + 1))
            ;;
    esac
done

if [[ -z "$input" || -z "$output" ]]; then
    echo "usage: $0 [cpp flags] input.c -o output.o" >&2
    exit 2
fi

cpp "${cpp_args[@]}" "$input" \
    | tools/gcc-2.7.2-psx/cc1 -O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000 -o "$output.s"
python3 tools/maspsx/maspsx.py --aspsx-version=2.86 --run-assembler \
    --gnu-as-path=mipsel-unknown-linux-gnu-as -G8 -o "$output" \
    -march=r3000 -mtune=r3000 -no-pad-sections -G8 -Iinclude "$output.s"
