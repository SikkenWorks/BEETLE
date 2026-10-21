#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O2 -Ifirmware/fm1-native tests/track/test_v078_nes.c firmware/fm1-native/track_chip.c firmware/fm1-native/track_mixer.c -o /tmp/beetle-v078-nes-test
/tmp/beetle-v078-nes-test
python3 tests/track/test_v078_ui.py
bash tests/track/run_v076.sh
