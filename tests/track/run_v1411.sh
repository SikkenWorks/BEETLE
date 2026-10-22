#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT="$(mktemp -d)"; trap 'rm -rf "$OUT"' EXIT
cc -std=c11 -O2 -g -fsanitize=undefined -Ifirmware/fm1-native \
 tests/track/test_v1411_song_and_drum.c \
 firmware/fm1-native/{track_core,track_song,track_chip,track_drum,track_mixer,beetle_drum_rom,track_project_store}.c \
 -o "$OUT/test_v1411"
UBSAN_OPTIONS=halt_on_error=1 "$OUT/test_v1411"
