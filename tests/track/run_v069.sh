#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/../.."
cc -std=c11 -O1 -g -fsanitize=undefined -Ifirmware/fm1-native \
 tests/track/test_beetle_v069_library.c \
 firmware/fm1-native/{track_core,track_song,track_mixer,track_chip,track_drum,beetle_drum_rom,track_project_store}.c \
 -o /tmp/beetle_v069_library
UBSAN_OPTIONS=halt_on_error=1 /tmp/beetle_v069_library
