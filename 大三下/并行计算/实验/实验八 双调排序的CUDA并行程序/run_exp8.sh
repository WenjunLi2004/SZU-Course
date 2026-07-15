#!/usr/bin/env bash
set -euo pipefail

SRC=/home/bxjs/2023150001/test/src/cuda/sort2.cu
OUT=/home/bxjs/2023150001/test/results/exp8.txt
BIN=/tmp/exp8_sort
RUNS=5
BLOCK_SIZE=1024

mkdir -p "$(dirname "$OUT")"
nvcc -O3 -std=c++11 "$SRC" -o "$BIN"

printf "# n block_size runs avg_time_seconds\n" > "$OUT"

for power in 23 24 25 26 27 28 29 30; do
    n=$((1 << power))
    sum=0

    for ((run = 1; run <= RUNS; run++)); do
        output="$("$BIN" "$n")"
        time_value="$(printf "%s\n" "$output" | awk '
            {
                for (i = 1; i <= NF; i++) {
                    if ($i ~ /^[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?$/) {
                        print $i;
                        exit;
                    }
                }
            }
        ')"
        if [[ -z "$time_value" ]]; then
            printf "程序没有输出可识别的时间数字：\n%s\n" "$output" >&2
            exit 1
        fi

        sum="$(awk -v s="$sum" -v t="$time_value" 'BEGIN { printf "%.12f", s + t }')"
        printf "n=%d run=%d/%d time=%s\n" "$n" "$run" "$RUNS" "$time_value" >&2
    done

    avg="$(awk -v s="$sum" -v r="$RUNS" 'BEGIN { printf "%.9f", s / r }')"
    printf "%d %d %d %s\n" "$n" "$BLOCK_SIZE" "$RUNS" "$avg" >> "$OUT"
done

printf "结果已保存到 %s\n" "$OUT" >&2
