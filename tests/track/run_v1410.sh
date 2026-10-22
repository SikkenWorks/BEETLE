#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT="$(mktemp -d)";trap 'rm -rf "$OUT"' EXIT
cc -std=c11 -O1 -g -fsanitize=undefined -Wall -Ifirmware/fm1-native \
 tests/track/test_v1410_mono_song.c \
 firmware/fm1-native/{track_core,track_chip,track_song,track_mixer,track_drum,beetle_drum_rom}.c \
 -o "$OUT/test_v1410"
UBSAN_OPTIONS=halt_on_error=1 "$OUT/test_v1410"
