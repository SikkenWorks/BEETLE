#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
C="-O2 -std=c11 -Wall -Wextra -Ifirmware/fm1-native"
cc $C tests/track/test_v12_wave_balance.c firmware/fm1-native/{track_chip,track_mixer}.c -lm -o "$work/wave"
"$work/wave"
cc $C tests/track/test_v12_usb_src.c -lm -o "$work/usb"
"$work/usb"
bash tests/track/run_rc1011b_audio_stability.sh
bash tests/track/run_rc11_help_usb.sh
python3 - <<'PYV'
from pathlib import Path
import re,json,hashlib,base64,subprocess,tempfile
root=Path('.')
manifest=json.loads((root/'FIRMWARE-MANIFEST.json').read_text())
raw=(root/'BEETLE-V1.2-USB-AUDIO-EXPERIMENTAL.fwsc').read_bytes()
assert manifest['firmware'].startswith('BEETLE V1.2')
assert manifest['sha256']==hashlib.sha256(raw).hexdigest()
assert manifest['checks']['header_same'] and manifest['checks']['loader_same'] and manifest['checks']['app_same']
assert manifest['identity']=='FM-1_2114' and manifest['schema']==19
for f in ('simulator.html','web/simulator.html'):
 text=(root/f).read_text(); assert 'BEETLE V1.2' in text
 scripts=re.findall(r'<script(?:\s[^>]*)?>(.*?)</script>',text,re.S)
 with tempfile.NamedTemporaryFile(mode='w',suffix='.js') as tmp:
  tmp.write('\n'.join(scripts));tmp.flush()
  subprocess.run(['node','--check',tmp.name],check=True,capture_output=True)
assert 'beetle_usb_src_next_step' in (root/'firmware/fm1-native/usb.c').read_text()
assert 'beetle_usb_src_lerp' in (root/'firmware/fm1-native/usb.c').read_text()
print('PASS V1.2 firmware+installer hash, native interpolation route, simulators JS parsing')
PYV
