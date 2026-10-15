#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O1 -g -fsanitize=undefined -fno-omit-frame-pointer -Ifirmware/fm1-native \
 tests/track/test_beetle_v067_snapshots.c \
 firmware/fm1-native/{track_core,track_song,track_mixer,track_chip,track_drum,beetle_drum_rom,track_project_store,track_journal}.c \
 -o /tmp/beetle_v067_snapshots
UBSAN_OPTIONS=halt_on_error=1 /tmp/beetle_v067_snapshots
bash tests/track/run_v065.sh
