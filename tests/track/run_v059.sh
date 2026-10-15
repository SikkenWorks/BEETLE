#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
OUT="${TMPDIR:-/tmp}/beetle_v059_test"
COMMON="track_core track_song track_mixer track_chip track_drum beetle_drum_rom track_project_store track_journal"
SOURCES=""
for n in $COMMON; do SOURCES="$SOURCES firmware/fm1-native/$n.c"; done
cc -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Ifirmware/fm1-native tests/track/test_beetle_v059_dynamics.c $SOURCES -o "$OUT"
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$OUT" tests/track/fixtures/beetle_v058_v10_project.trk
