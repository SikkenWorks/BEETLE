#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -Wall -Wextra -Werror -O2 -Ifirmware/fm1-native tests/track/test_v079_sel_guard.c -o /tmp/beetle-v079-sel
/tmp/beetle-v079-sel
python3 tests/track/test_v079_sel_static.py
bash tests/track/run_v078.sh
