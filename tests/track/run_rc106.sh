#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/../.."
cc -std=c11 -O1 -g -fsanitize=undefined -Ifirmware/fm1-native \
 tests/track/test_rc106_note2_fx.c \
 firmware/fm1-native/{track_core,track_song,track_mixer,track_chip,track_drum,beetle_drum_rom,track_project_store}.c \
 -o /tmp/beetle_rc106_note2
UBSAN_OPTIONS=halt_on_error=1 /tmp/beetle_rc106_note2 tests/project-manager/native_fixture_v17.trk
