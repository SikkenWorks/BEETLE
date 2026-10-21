#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${CC:=clang}"
"$CC" -std=c11 -O1 -g -Wall -Wextra -Werror -fno-omit-frame-pointer \
 -fsanitize=address,undefined -I firmware/fm1-native \
 tests/track/test_beetle_v046.c \
 firmware/fm1-native/{track_chip,track_core,track_mixer,track_drum,beetle_drum_rom,track_song,track_project_store,track_journal}.c \
 -o /tmp/beetle-v046-test
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 /tmp/beetle-v046-test
bash tests/track/run_v045.sh
