#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
bin="$(mktemp /tmp/beetle-usb-six-XXXXXX)"
trap 'rm -f "$bin"' EXIT
C=(-std=c11 -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -I firmware/fm1-native)
cc "${C[@]}" tests/usb/test_multitrack_realtime.c -o "$bin"
"$bin"
cc "${C[@]}" tests/usb/test_multitrack_clock.c -o "$bin"
"$bin"
