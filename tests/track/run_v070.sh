#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT="$(mktemp -d)";trap 'rm -rf "$OUT"' EXIT
cc -O1 -g -std=c11 -Ifirmware/fm1-native tests/track/test_v070_unified_history.c firmware/fm1-native/track_core.c firmware/fm1-native/track_song.c -o "$OUT/v070_history"
"$OUT/v070_history"
cc -O1 -g -std=c11 -Ifirmware/fm1-native tests/track/test_v056_edit.c firmware/fm1-native/track_core.c firmware/fm1-native/track_song.c -o "$OUT/v056_history"
"$OUT/v056_history"
