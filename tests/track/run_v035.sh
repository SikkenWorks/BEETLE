#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
: "${CC:=cc}"
FLAGS=(-std=c11 -O1 -g -fno-omit-frame-pointer -I firmware/fm1-native)
if [[ "${SANITIZE:-0}" == 1 ]]; then FLAGS+=(-fsanitize=address,undefined); fi
"$CC" "${FLAGS[@]}" tests/track/test_beetle_v035.c firmware/fm1-native/{track_core,track_song,track_mixer,track_project_store,track_chip,track_drum,beetle_drum_rom,track_journal,beetle_flash_layout}.c -o /tmp/beetle-v035-test
if [[ -f tests/track/legacy-v031-project-v3.bin && -f tests/track/legacy-v034-project-v4.bin ]];then
 /tmp/beetle-v035-test tests/track/legacy-v031-project-v3.bin tests/track/legacy-v034-project-v4.bin
else
 echo 'NOTICE: V0.31/V0.34 legacy fixtures were not included in the supplied V0.37 release; testing current V7 journal without external binary fixtures.'
 /tmp/beetle-v035-test
fi
