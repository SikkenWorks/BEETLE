#!/usr/bin/env bash
# Run 64 consecutive NUGGIE project loops for stored CH2 and 12 override types.
# Host-only deterministic PCM/voice count check. NO device access.
set -euo pipefail
if [[ $# != 1 ]]; then echo "Usage: $0 NUGGIE-SLOT-07.beetle or decoded.raw" >&2; exit 2; fi
cd "$(dirname "$0")/../.."
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
raw="$1"
if [[ "$raw" == *.beetle ]]; then
  python tools/diagnostics/decrypt_beetle_v2.py "$raw" "$work/private.raw"
  raw="$work/private.raw"
fi
src=firmware/fm1-native
cc -O2 -std=c11 -I"$src" tests/diagnostics/benchmark_nuggie.c \
 "$src/track_project_store.c" "$src/track_core.c" "$src/track_chip.c" \
 "$src/track_drum.c" "$src/beetle_drum_rom.c" "$src/track_mixer.c" -o "$work/nuggie"
for type in -1 $(seq 0 11); do "$work/nuggie" "$raw" "$type" 64; done
