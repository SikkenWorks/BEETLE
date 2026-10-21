#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O2 -Ifirmware/fm1-native tests/track/test_v080_dmg_pulse.c firmware/fm1-native/track_chip.c firmware/fm1-native/track_mixer.c -o /tmp/beetle-v080-dmg-test
/tmp/beetle-v080-dmg-test
python3 tests/track/test_v080_drum_preview.py
bash tests/track/run_v079.sh
