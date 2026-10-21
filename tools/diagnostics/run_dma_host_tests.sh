#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
work="$(mktemp -d)";trap 'rm -rf "$work"' EXIT
src=firmware/fm1-native
objs="$src/track_chip.c $src/track_drum.c $src/beetle_drum_rom.c $src/track_mixer.c"
cc -O2 -std=c11 -I"$src" tests/diagnostics/test_dma_pcm_matrix.c $objs -o "$work/matrix"
actual="$("$work/matrix")";echo "$actual"
test "$actual" = 'PCM_MATRIX_FNV df02ac8b' || { echo 'PCM regression' >&2;exit 1; }
cc -O2 -std=c11 -fsanitize=undefined -fno-sanitize-recover=undefined -I"$src" tests/diagnostics/test_dma_host_soak.c $objs -o "$work/soak"
UBSAN_OPTIONS=halt_on_error=1 "$work/soak"
cc -O2 -std=c11 -I"$src" tests/diagnostics/benchmark_nuggie.c $src/track_project_store.c $src/track_core.c $objs -o "$work/nuggie"
if [[ $# -gt 0 ]];then
 echo 'NUGGIE project: native project decode, two-loop audio, 12 CH2 oscillator variants'
 for t in -1 $(seq 0 11);do "$work/nuggie" "$1" "$t";done
else
 echo 'Optional project test: decrypt .beetle to a private .raw, then pass it as argument.'
fi
