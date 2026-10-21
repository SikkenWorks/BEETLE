#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
work="$(mktemp -d)";trap 'rm -rf "$work"' EXIT
C="-O2 -std=c11 -Wall -Wextra -Ifirmware/fm1-native"
cc $C tests/track/test_v13_punch_song.c firmware/fm1-native/{track_song,track_core,track_drum,beetle_drum_rom,track_chip,track_mixer}.c -lm -o "$work/punch"
"$work/punch"
cc $C tests/track/test_v13_poly_pan.c firmware/fm1-native/{track_chip,track_mixer,track_drum,beetle_drum_rom}.c -lm -o "$work/pan"
"$work/pan"
bash tests/track/run_v13_audio_stability.sh
cc $C tests/track/test_v12_wave_balance.c firmware/fm1-native/{track_chip,track_mixer}.c -lm -o "$work/wave"
"$work/wave"
cc $C tests/track/test_v12_usb_src.c -lm -o "$work/usb"
"$work/usb"
cc $C tests/track/test_rc11_help_chapters.c -o "$work/help"
"$work/help"
python3 - <<'PY'
from pathlib import Path
import re,json,hashlib,base64,subprocess,tempfile
s=Path('firmware/fm1-native/audio_target.c').read_text()
assert 'native_usb_stereo[2*i]=left;native_usb_stereo[2*i+1]=right;' in s
assert 'uac_tap(native_usb_stereo,ETCH_NATIVE_FRAMES)' in s
u=Path('firmware/fm1-native/usb.c').read_text()
assert 'uac_s16(out[2u * i + 1u])' in u
n=Path('firmware/fm1-native/track_runtime.h').read_text()
assert 'track_song_punch_pattern(&track_song,&track_state)' in n
assert 'track_recorded_punch_source[i]' in n
assert 'track_recorded_punch_edited[source]' in n
assert 'track_song_effect_mask_boundary' in n
items=re.findall(r'"([^"\\]*)"',Path('firmware/fm1-native/beetle_help.h').read_text().split('static const char *const beetle_help_lines[] = {')[1].split('};')[0])
for f in ('simulator.html','web/simulator.html'):
 s=Path(f).read_text()
 assert "text('INST '+" in s and 'punchRecorded:new Map()' in s
 a=re.search(r'const HELP_LINES=(\[.*?\]);',s)
 assert a and json.loads(a.group(1))==items
 scripts=re.findall(r'<script(?:\s[^>]*)?>(.*?)</script>',s,re.S)
 with tempfile.NamedTemporaryFile('w',suffix='.js') as tmp:
  tmp.write('\n'.join(scripts));tmp.flush()
  subprocess.run(['node','--check',tmp.name],check=True,capture_output=True)
raw=Path('BEETLE-V1.3-USB-AUDIO-EXPERIMENTAL.fwsc').read_bytes()
m=json.loads(Path('FIRMWARE-MANIFEST.json').read_text())
assert m['sha256']==hashlib.sha256(raw).hexdigest()
assert m['identity']=='FM-1_2115' and m['schema']==19
assert all(m['checks'].values())
installer=Path('WebInstaller.html').read_text()
assert "HASH_ET: '"+m['sha256']+"'" in installer
assert "ID_ET: 'FM-1_2115'" in installer
b64=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',installer,re.S)
assert b64 and base64.b64decode(b64.group(1))==raw
print('PASS V1.3: SHA-pinned firmware+installer, loader/header/app audit, 12 HELP chapters, JS syntax')
print('PASS V1.3: post-master native stereo routed without downmix to UAC; Song and simulator code checks')
PY
