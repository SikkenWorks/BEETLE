#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT="$(mktemp -d)"; trap 'rm -rf "$OUT"' EXIT
cc -std=c11 -O1 -g -fsanitize=undefined -fno-sanitize-recover=all \
 -Ifirmware/fm1-native tests/track/test_v073_rc_diagnostics.c -o "$OUT/v073_rc"
UBSAN_OPTIONS=halt_on_error=1 "$OUT/v073_rc"
python tests/track/test_v073_ui.py
bash tests/track/run_v072.sh
