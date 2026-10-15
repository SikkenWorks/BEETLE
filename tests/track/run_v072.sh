#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
cc -std=c11 -O1 -g -fsanitize=undefined -fno-sanitize-recover=all \
 -Ifirmware/fm1-native tests/track/test_v072_audio_rc.c \
 firmware/fm1-native/{track_core,track_chip,track_drum,beetle_drum_rom,track_mixer}.c \
 -o "$OUT/audio_rc"
UBSAN_OPTIONS=halt_on_error=1 "$OUT/audio_rc"
bash tests/track/run_v071.sh
