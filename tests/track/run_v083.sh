#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -Wall -Wextra -Wno-error=misleading-indentation -O2 -Ifirmware/fm1-native tests/track/test_v083_octave.c firmware/fm1-native/track_chip.c firmware/fm1-native/track_mixer.c -o /tmp/beetle-v083-octave
/tmp/beetle-v083-octave
python3 tests/track/test_v083_patch_focus.py
bash tests/track/run_v082.sh
