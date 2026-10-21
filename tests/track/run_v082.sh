#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -Wall -Wextra -Werror -O2 -Ifirmware/fm1-native tests/track/test_v082_punch_select.c -o /tmp/beetle-v082-punch-select
/tmp/beetle-v082-punch-select
bash tests/track/run_v080.sh
