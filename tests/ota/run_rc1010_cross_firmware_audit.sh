#!/usr/bin/env bash
# Native host OTA packet replay, no physical FM1 required. GPL-3.0-only.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
python3 tests/ota/make_cross_firmware_fixtures.py
cc -std=c11 -O2 -w -Ifirmware/fm1-native tests/ota/native_cross_firmware_replay.c \
   firmware/fm1-native/native_digest.c -o tests/ota/generated/native_cross_firmware_replay
run() {
  echo "[NATIVE OTA] $1 expected $2 commit $3"
  tests/ota/generated/native_cross_firmware_replay "tests/ota/generated/$1.logical.bin" "$2" "$3"
}
run beetle_rc1010 0 1
run compatible_foreign_head 0 1
run ETCH-V1.36-ORIGINAL 0 1
run ETCH-FM1-V1.37-ORIGINAL 0 1
run corrupt_app_payload -24 0
run wrong_app_xip -23 0
run untrusted_loader -16 0
python3 - <<'PY'
from pathlib import Path
import sys
sys.path.insert(0,'tools')
import fm1_fwsc as fw
p=Path('BEETLE-1.01-RC1.0.10-EXPERIMENTAL.fwsc')
report,errors=fw.verify_fwsc(p)
assert not errors,errors
assert report['product']=='FM-1_2110'
Path('tests/ota/generated/final_installer_image.logical.bin').write_bytes(fw.decode_fwsc(p.read_bytes())[1])
print('[PACKAGE] RC1.0.10 .fwsc CRC and layout PASS')
PY
run final_installer_image 0 1
node tests/ota/test_rc1010_app_only.cjs
echo 'PASS: native C CHECK replay, actual ETCH sources, corrupted app and loader rejection, browser verifier parity.'
