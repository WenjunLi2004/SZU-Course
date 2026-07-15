#!/bin/zsh
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -d ".venv" ]; then
  /usr/bin/python3 -m venv .venv
fi

source ".venv/bin/activate"
python -m notebook
