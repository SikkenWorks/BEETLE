#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O2 -I firmware/fm1-native tests/track/test_rc1_live_mixer.c -o /tmp/beetle-rc1-live
/tmp/beetle-rc1-live
bash tests/track/run_v087.sh
