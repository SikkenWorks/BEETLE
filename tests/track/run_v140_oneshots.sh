#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O2 -g -fsanitize=undefined -fno-omit-frame-pointer -I firmware/fm1-native tests/track/test_v140_oneshots.c firmware/fm1-native/{track_core,track_song,track_mixer,track_chip,track_drum,beetle_drum_rom,track_project_store}.c -o /tmp/beetle-v140-oneshots
/tmp/beetle-v140-oneshots
