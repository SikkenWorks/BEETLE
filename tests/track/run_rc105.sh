#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O2 -Wall -Wextra -Wno-unused-function -I firmware/fm1-native \
 tests/track/test_rc105_full_wipe_host.c firmware/fm1-native/beetle_flash_layout.c -o /tmp/beetle_rc105_wipe
/tmp/beetle_rc105_wipe
bash tests/track/run_rc103.sh
bash tests/project_transfer/run_delete_tests.sh
node tests/project_transfer/test_web_crypto.cjs
