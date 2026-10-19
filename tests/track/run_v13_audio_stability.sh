#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
C='-O1 -g -std=c11 -fsanitize=undefined -fno-omit-frame-pointer -Ifirmware/fm1-native'
cc $C tests/track/test_rc1011b_audio_stability.c firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer}.c -o "$work/song"
UBSAN_OPTIONS=halt_on_error=1 "$work/song"
cc $C tests/track/test_rc1011b_pcm_compat.c firmware/fm1-native/{track_chip,track_mixer}.c -o "$work/pcm"
# V1.2 deliberately changes PCM gain calibration; original RC reference was 2ccc77fc.
expected='PCM FNV1A32 d98a5e13'
actual="$($work/pcm)"
[ "$actual" = "$expected" ] || { echo "PCM regression failed: $actual != $expected"; exit 1; }
echo "PASS V1.3 intentional polyphonic PCM balance, intentional calibrated PCM checksum: $actual"
cc $C tests/track/test_rc1011_song_chains.c firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer,track_project_store}.c -o "$work/v19"
UBSAN_OPTIONS=halt_on_error=1 "$work/v19"
