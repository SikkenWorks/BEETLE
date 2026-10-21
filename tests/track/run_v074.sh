#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT="$(mktemp -d)"; trap 'rm -rf "$OUT"' EXIT
cc -std=c11 -O1 -g -Wall -Wextra -fsanitize=undefined -fno-sanitize-recover=all \
 -Ifirmware/fm1-native tests/track/test_v074_store_preflight.c \
 firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer,track_project_store,track_journal}.c \
 -o "$OUT/preflight"
UBSAN_OPTIONS=halt_on_error=1 "$OUT/preflight"
bash tests/track/run_v073.sh
