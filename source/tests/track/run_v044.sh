#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
CC="${CC:-clang}"
"$CC" -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 -I firmware/fm1-native tests/track/test_beetle_v044_pcm.c \
 firmware/fm1-native/{track_drum,beetle_drum_rom,track_mixer}.c -o /tmp/beetle-v044-rom-test
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 /tmp/beetle-v044-rom-test
bash tests/track/run_v043.sh
