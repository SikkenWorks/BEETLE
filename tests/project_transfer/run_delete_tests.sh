#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cc -std=c11 -O1 -g -fsanitize=undefined -fno-omit-frame-pointer -I firmware/fm1-native tests/project_transfer/test_delete_native.c firmware/fm1-native/{track_core,track_mixer,track_song,track_chip,track_journal,track_drum,beetle_drum_rom,track_name,track_project_store,beetle_flash_layout}.c -o /tmp/beetle-v2-delete-test
/tmp/beetle-v2-delete-test
node tests/project_transfer/test_web_crypto.cjs
python tests/project_transfer/test_delete_browser.py

python tests/project_transfer/test_rc104_marker_browser.py
