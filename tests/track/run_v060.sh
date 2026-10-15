#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
OUT="${TMPDIR:-/tmp}/beetle_v060_workflow"
cc -std=c11 -O2 -Ifirmware/fm1-native tests/track/test_beetle_v060_workflow.c firmware/fm1-native/track_core.c firmware/fm1-native/track_song.c -o "$OUT"
"$OUT"
