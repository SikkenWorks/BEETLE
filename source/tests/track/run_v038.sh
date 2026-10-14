#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
: "${CC:=cc}"
"$CC" -std=c11 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -I firmware/fm1-native \
 tests/track/test_beetle_v038.c firmware/fm1-native/{track_core,track_drum,beetle_drum_rom,track_chip,track_mixer,track_song,track_project_store}.c \
 -o /tmp/beetle-v038-test
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=detect_leaks=0 /tmp/beetle-v038-test
SANITIZE=1 bash tests/track/run_v037.sh
