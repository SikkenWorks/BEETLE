#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT="$(mktemp -d)";trap 'rm -rf "$OUT"' EXIT
cc -O1 -g -std=c11 -fsanitize=undefined -Ifirmware/fm1-native \
 tests/track/test_rc1010_storage_workspace.c \
 firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer,track_project_store,track_journal}.c \
 -o "$OUT/rc1010_storage"
UBSAN_OPTIONS=halt_on_error=1 "$OUT/rc1010_storage"
