#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT="$(mktemp -d)";trap 'rm -rf "$OUT"' EXIT
cc -O1 -g -std=c11 -fsanitize=undefined -Ifirmware/fm1-native \
  tests/track/test_v071_journal_recovery.c \
  firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer,track_project_store,track_journal}.c \
  -o "$OUT/v071_journal"
UBSAN_OPTIONS=halt_on_error=1 "$OUT/v071_journal"
bash tests/track/run_v070.sh
bash tests/track/run_v069.sh
python tests/web/test_v071_backup.py
python tests/track/test_v071_backup_audit.py
