#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)";cd "$ROOT"
bash tests/track/run_v036.sh
FLAGS=(-std=c11 -O1 -g -fno-omit-frame-pointer -I firmware/fm1-native)
if [[ "${SANITIZE:-0}" == 1 ]];then FLAGS+=(-fsanitize=address,undefined);fi
cc "${FLAGS[@]}" tests/track/test_beetle_v037_audio.c firmware/fm1-native/{track_core,track_drum,beetle_drum_rom,track_chip,track_mixer,track_song,track_project_store}.c -o /tmp/beetle-v037-audio
/tmp/beetle-v037-audio
