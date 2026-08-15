#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export OVSBOS_ROOT="$SCRIPT_DIR"

if ! command -v python3 >/dev/null 2>&1; then
  echo "[setup] python3 nao encontrado. Instale python3 para iniciar a interface TUI." >&2
  exit 1
fi

exec python3 "$SCRIPT_DIR/scripts/ovsbos_tui.py" setup "$@"
