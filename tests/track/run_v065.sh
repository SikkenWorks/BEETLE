#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O1 -g -fsanitize=undefined -fno-omit-frame-pointer -Ifirmware/fm1-native tests/track/test_beetle_v065_mute.c -o /tmp/beetle-v065-mute
UBSAN_OPTIONS=halt_on_error=1 /tmp/beetle-v065-mute
bash tests/track/run_v064.sh
