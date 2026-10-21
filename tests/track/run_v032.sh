#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="$ROOT/firmware/fm1-native"
TEST="$ROOT/tests/track"
BIN="$(mktemp /tmp/beetle-v032-test.XXXXXX)"
trap 'rm -f "$BIN"' EXIT
CFLAGS=(-std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I"$SRC")
clang "${CFLAGS[@]}" "$TEST/test_beetle_v032.c" "$SRC"/{track_core.c,track_chip.c,track_mixer.c,track_song.c,track_project_store.c} -o "$BIN"
"$BIN" "$TEST/legacy-v031-project-v3.bin"
clang "${CFLAGS[@]}" "$TEST/test_beetle_v031.c" "$SRC"/{track_core.c,track_mixer.c,track_song.c,track_chip.c,track_journal.c,track_drum.c,beetle_drum_rom.c,track_name.c,track_project_store.c,beetle_flash_layout.c} -o "$BIN"
"$BIN"
clang "${CFLAGS[@]}" "$TEST/test_beetle_v030.c" "$SRC"/{track_core.c,track_chip.c,track_drum.c,beetle_drum_rom.c,track_mixer.c} -o "$BIN"
"$BIN"
clang "${CFLAGS[@]}" "$TEST/test_beetle_v029.c" "$SRC"/{track_core.c,track_chip.c,track_drum.c,beetle_drum_rom.c,track_mixer.c} -o "$BIN"
"$BIN"
clang "${CFLAGS[@]}" "$TEST/test_v023_max_capacity.c" "$SRC"/{track_core.c,track_chip.c,track_song.c,track_mixer.c,track_project_store.c} -o "$BIN"
"$BIN"
echo 'BEETLE V0.32 targeted native regressions: PASS'
