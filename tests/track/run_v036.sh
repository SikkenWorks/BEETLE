#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
: "${CC:=cc}"
FLAGS=(-std=c11 -O1 -g -fno-omit-frame-pointer -I firmware/fm1-native)
if [[ "${SANITIZE:-0}" == 1 ]]; then FLAGS+=(-fsanitize=address,undefined); fi
"$CC" "${FLAGS[@]}" tests/track/test_beetle_v036.c firmware/fm1-native/{track_core,track_mixer}.c -o /tmp/beetle-v036-ui-test
/tmp/beetle-v036-ui-test
bash tests/track/run_v035.sh
"$CC" "${FLAGS[@]}" tests/track/test_beetle_v036_va.c firmware/fm1-native/{track_core,track_song,track_mixer,track_chip,track_drum,beetle_drum_rom,track_project_store}.c -o /tmp/beetle-v036-va-test
/tmp/beetle-v036-va-test
