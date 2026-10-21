#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -Wall -Wextra -Werror -O2 tests/track/test_rc103_led_ripple.c -o /tmp/beetle_rc103_led_test
/tmp/beetle_rc103_led_test
