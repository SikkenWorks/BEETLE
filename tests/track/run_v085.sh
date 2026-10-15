#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O2 -Ifirmware/fm1-native tests/track/test_v085_note_octave.c firmware/fm1-native/track_core.c -o /tmp/beetle-v085-note-oct
/tmp/beetle-v085-note-oct
python3 tests/track/test_v085_routes.py
bash tests/track/run_v084.sh
