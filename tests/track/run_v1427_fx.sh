#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O2 -fsanitize=undefined -fno-sanitize-recover=all \
 -Ifirmware/fm1-native tests/track/test_v1427_bitcrush_radio.c \
 firmware/fm1-native/track_chip.c firmware/fm1-native/track_mixer.c -lm \
 -o /tmp/beetle_v1427_fx
/tmp/beetle_v1427_fx
cc -std=c11 -O2 -fsanitize=undefined -fno-sanitize-recover=all \
 -Ifirmware/fm1-native tests/track/test_v1426_audio_note2.c \
 firmware/fm1-native/track_chip.c firmware/fm1-native/track_mixer.c \
 firmware/fm1-native/track_core.c firmware/fm1-native/track_drum.c \
 firmware/fm1-native/beetle_drum_rom.c -lm -o /tmp/beetle_v1426_regression
/tmp/beetle_v1426_regression
