#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/../.."
cc -std=c11 -O2 -g -fsanitize=undefined -fno-omit-frame-pointer -DBEETLE_NATIVE_STORAGE=1 -Ifirmware/fm1-native tests/track/test_v140_journal_oneshots.c firmware/fm1-native/{track_journal,track_project_store,track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer}.c -o /tmp/beetle-v140-journal-oneshots
/tmp/beetle-v140-journal-oneshots
