#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${CC:=clang}"
"$CC" -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -fno-omit-frame-pointer -I firmware/fm1-native \
 tests/track/test_beetle_v043_factory.c \
 firmware/fm1-native/track_drum.c firmware/fm1-native/beetle_drum_rom.c firmware/fm1-native/track_mixer.c \
 -o /tmp/beetle-v043-factory-test
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 /tmp/beetle-v043-factory-test
bash tests/track/run_v041.sh
