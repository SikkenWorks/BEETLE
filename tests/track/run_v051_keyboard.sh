#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
clang -std=c11 -O2 -Wall -Wextra -Werror -I firmware/fm1-native tests/track/test_beetle_v051_keyboard.c -o /tmp/beetle-v051-keyboard
/tmp/beetle-v051-keyboard
python tests/track/test_beetle_v051_native.py
python tests/track/test_v051_identity_boot.py
