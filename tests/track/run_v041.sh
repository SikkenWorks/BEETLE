#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)";cd "$ROOT"
: "${CC:=cc}"
"$CC" -std=c11 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -I firmware/fm1-native \
 tests/track/test_beetle_v041.c firmware/fm1-native/{track_core,track_drum,beetle_drum_rom,track_chip,track_mixer,track_song,track_project_store}.c \
 -o /tmp/beetle-v041-test
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 /tmp/beetle-v041-test
bash tests/track/run_v040.sh
