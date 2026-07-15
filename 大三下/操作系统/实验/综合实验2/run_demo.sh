#!/usr/bin/env bash
set -euo pipefail

make clean
make
./sfs | tee demo_output.txt
