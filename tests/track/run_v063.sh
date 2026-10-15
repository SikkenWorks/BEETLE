#!/bin/bash
set -eu
cd "$(dirname "$0")/../.."
cc -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Ifirmware/fm1-native \
 tests/track/test_beetle_v063_groove.c \
 firmware/fm1-native/{track_core,track_song,track_mixer,track_chip,track_drum,beetle_drum_rom,track_project_store,track_journal}.c \
 -o /tmp/beetle_v063_groove
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 /tmp/beetle_v063_groove tests/track/fixtures/beetle_v058_v10_project.trk
