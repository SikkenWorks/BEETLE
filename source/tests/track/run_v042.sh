#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
bash tests/track/run_v041.sh
CC="${CC:-clang}"
"$CC" -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Ifirmware/fm1-native tests/track/test_beetle_v042_visual.c -o /tmp/beetle_v042_visual_test
/tmp/beetle_v042_visual_test
python tools/beetle_v042_static_ota_audit.py
