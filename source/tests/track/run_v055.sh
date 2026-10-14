#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${CC:=clang}"
COMMON=( -std=c11 -O1 -g -Wall -Wextra -Werror -fno-omit-frame-pointer \
 -fsanitize=address,undefined -I firmware/fm1-native )
"$CC" "${COMMON[@]}" tests/track/test_beetle_v053_sid_character.c \
 firmware/fm1-native/{track_chip,track_core,track_mixer}.c -o /tmp/beetle-v054-sid-character
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 /tmp/beetle-v054-sid-character
"$CC" "${COMMON[@]}" tests/track/test_beetle_v052.c \
 firmware/fm1-native/{track_chip,track_core,track_mixer,track_drum,beetle_drum_rom,track_song,track_project_store,track_journal}.c \
 -o /tmp/beetle-v054-regression
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 /tmp/beetle-v054-regression
cc -std=c11 -O2 -Wall -Wextra -Werror -I firmware/fm1-native tests/track/test_beetle_v054_help.c -o /tmp/beetle-v054-help
/tmp/beetle-v054-help
python tests/web/test_beetle_v054_browser.py
python tests/web/test_beetle_v054_previous_browser.py
python tests/web/test_beetle_v054_sid_color.py
python tests/web/test_beetle_v055_layout_navigation.py
node tests/ota/test_stock_install_v055.cjs
