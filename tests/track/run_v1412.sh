#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
cc -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/fm1-native tests/track/test_v1412_song_rows_battery.c -o /tmp/beetle_v1412_song_rows
/tmp/beetle_v1412_song_rows
bash tests/track/run_v1411.sh
