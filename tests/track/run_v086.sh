#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O2 -Ifirmware/fm1-native tests/track/test_v086_live_mixer.c firmware/fm1-native/{track_core,track_mixer,track_song,track_project_store,track_chip,track_drum,beetle_drum_rom}.c -o /tmp/beetle-v086-live-mixer
/tmp/beetle-v086-live-mixer
bash tests/track/run_v085.sh
