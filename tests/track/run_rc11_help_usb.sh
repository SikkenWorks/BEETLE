#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
cc -std=c11 -O2 -Wall -Wextra tests/track/test_rc11_help_chapters.c -o "$tmp/help"
"$tmp/help"
python3 - <<'PY'
import re,json,hashlib,base64
from pathlib import Path
head=Path('firmware/fm1-native/beetle_help.h').read_text()
items=re.findall(r'"([^"\\]*)"',head.split('static const char *const beetle_help_lines[] = {')[1].split('};')[0])
for fn in ('simulator.html','web/simulator.html'):
 text=Path(fn).read_text()
 a=re.search(r'const HELP_LINES=(\[.*?\]);',text)
 assert a and json.loads(a.group(1))==items, fn
 assert 'helpScrollMax(model.helpChapter)' in text
 assert 'model.helpChapter+dir' in text
print('PASS WebSimulator content and controls mirror native HELP')
raw=Path('BEETLE-1.01-RC1.1-USB-AUDIO-EXPERIMENTAL.fwsc').read_bytes()
sha=hashlib.sha256(raw).hexdigest()
installer=Path('WebInstaller.html').read_text()
assert "HASH_ET: '"+sha+"'" in installer
assert "ID_ET: 'FM-1_2113'" in installer
b64=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',installer,re.S)
assert b64 and base64.b64decode(b64.group(1))==raw
assert Path('FIRMWARE-MANIFEST.json').read_text().find(sha)>=0
src=Path('firmware/fm1-native/audio_target.c').read_text()
assert 'uac_tap(native_usb_stereo,ETCH_NATIVE_FRAMES)' in src
assert 'ETCH_NATIVE_UAC' in Path('firmware/fm1-native/bringup.c').read_text()
print('PASS USB UAC source tap, firmware hash, embedded installer and new identity')
PY
