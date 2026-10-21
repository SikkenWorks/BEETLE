#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O2 -Ifirmware/fm1-native tests/track/test_v084_punch.c firmware/fm1-native/track_chip.c firmware/fm1-native/track_mixer.c -o /tmp/beetle-v084-punch
/tmp/beetle-v084-punch
python3 tests/track/test_v084_ui.py
bash tests/track/run_v083.sh
