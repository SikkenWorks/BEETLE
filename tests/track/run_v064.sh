#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O1 -g -fsanitize=undefined -fno-omit-frame-pointer -Ifirmware/fm1-native tests/track/test_beetle_v064_groove_tools.c -o /tmp/beetle-v064-tools
UBSAN_OPTIONS=halt_on_error=1 /tmp/beetle-v064-tools
bash tests/track/run_v063.sh
