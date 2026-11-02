#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Host-only BEETLE V1.8.1 realtime math and safety regression checks.
set -euo pipefail
cd "$(dirname "$0")/../.."
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
cc -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=undefined \
  -Ifirmware/fm1-native tests/track/test_v181_master_math.c -o "$tmp/master"
UBSAN_OPTIONS=halt_on_error=1 "$tmp/master"
cc -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=undefined \
  -Ifirmware/fm1-native tests/track/test_v1611_audio_safety.c -o "$tmp/master_safety"
UBSAN_OPTIONS=halt_on_error=1 "$tmp/master_safety"
bash tests/track/run_rc1011b_audio_stability.sh
bash tests/track/run_rc1010_storage_workspace.sh
python tests/dpcm4/run_v1422_live.py
