#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
cc -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/fm1-native tests/track/test_v1413_rec_led.c -o /tmp/beetle_v1413_rec_led
/tmp/beetle_v1413_rec_led
bash tests/track/run_v1412.sh
