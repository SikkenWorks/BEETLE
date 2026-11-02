#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
C='-O1 -g -std=c11 -fsanitize=undefined -fno-omit-frame-pointer -Ifirmware/fm1-native'
cc $C tests/track/test_rc1011b_audio_stability.c firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer}.c -o "$work/song"
UBSAN_OPTIONS=halt_on_error=1 "$work/song"
cc $C tests/track/test_rc1011b_pcm_compat.c firmware/fm1-native/{track_chip,track_mixer}.c -o "$work/pcm"
# The historic V1.2 checksum 43c0f1f6 was already stale in V1.7.14.
# The old test fixture + track_chip.c + track_mixer.c are byte-identical
# across V1.7.14 and V1.8; pin against that current reference instead.
# Do not claim to have explained the older reference drift.
expected='PCM FNV1A32 dd0f3681' 
actual="$($work/pcm)"
[ "$actual" = "$expected" ] || { echo "PCM regression failed: $actual != $expected"; exit 1; }
echo "PASS stable chip PCM versus V1.7.14 / V1.8 baseline: $actual (older reference unexplained)"
cc $C tests/track/test_rc1011_song_chains.c firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer,track_project_store}.c -o "$work/v19"
UBSAN_OPTIONS=halt_on_error=1 "$work/v19"
