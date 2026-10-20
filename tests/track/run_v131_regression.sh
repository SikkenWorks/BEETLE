#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
WORK="$(mktemp -d)";trap 'rm -rf "$WORK"' EXIT
C='-O1 -g -std=c11 -fsanitize=undefined -fno-omit-frame-pointer -Ifirmware/fm1-native'
cc $C tests/track/test_v131_patch_slots.c firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer,track_project_store}.c -o "$WORK/patchslots"
UBSAN_OPTIONS=halt_on_error=1 "$WORK/patchslots" tests/fixtures/BEETLE-V19-real-legacy-project.trk "$WORK/latest-v20.trk"
node tests/track/test_v131_web_codec.cjs tests/fixtures/BEETLE-V19-real-legacy-project.trk "$WORK/latest-v20.trk"
cc $C tests/track/test_v13_punch_song.c firmware/fm1-native/{track_song,track_core,track_drum,beetle_drum_rom,track_chip,track_mixer}.c -lm -o "$WORK/punch"
UBSAN_OPTIONS=halt_on_error=1 "$WORK/punch"
cc $C tests/track/test_v13_poly_pan.c firmware/fm1-native/{track_chip,track_mixer,track_drum,beetle_drum_rom}.c -lm -o "$WORK/pan"
UBSAN_OPTIONS=halt_on_error=1 "$WORK/pan"
cc $C tests/track/test_rc1011b_audio_stability.c firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer}.c -o "$WORK/song"
UBSAN_OPTIONS=halt_on_error=1 "$WORK/song"
cc $C tests/track/test_rc1011b_pcm_compat.c firmware/fm1-native/{track_chip,track_mixer}.c -o "$WORK/pcm"
[ "$($WORK/pcm)" = 'PCM FNV1A32 fbda64f6' ] || { echo 'UNEXPECTED PCM CHANGE'; exit 1; }
cc $C tests/track/test_v12_wave_balance.c firmware/fm1-native/{track_chip,track_mixer}.c -lm -o "$WORK/wave"
"$WORK/wave"
cc $C tests/track/test_v12_usb_src.c -lm -o "$WORK/usb"
"$WORK/usb"
cc $C tests/track/test_rc11_help_chapters.c -o "$WORK/help"
"$WORK/help"
python3 - <<'PY'
from pathlib import Path
import re,json,hashlib,base64,subprocess,tempfile
h=Path('firmware/fm1-native/beetle_help.h').read_text()
items=re.findall(r'"([^"\\]*)"',h.split('static const char *const beetle_help_lines[] = {')[1].split('};')[0])
for file in ('simulator.html','web/simulator.html'):
 s=Path(file).read_text()
 a=re.search(r'const HELP_LINES=(\[.*?\]);',s)
 assert a and json.loads(a.group(1))==items
 scripts=re.findall(r'<script(?:\s[^>]*)?>(.*?)</script>',s,re.S)
 with tempfile.NamedTemporaryFile('w',suffix='.js') as tmp:
  tmp.write('\n'.join(scripts));tmp.flush()
  subprocess.run(['node','--check',tmp.name],check=True)
raw=Path('BEETLE-V1.3.2-USB-AUDIO-EXPERIMENTAL.fwsc').read_bytes()
m=json.loads(Path('FIRMWARE-MANIFEST.json').read_text())
assert m['identity']=='FM-1_2117' and m['schema']==20
assert m['sha256']==hashlib.sha256(raw).hexdigest()
assert all(m['checks'].values())
w=Path('WebInstaller.html').read_text()
assert "HASH_ET: '"+m['sha256']+"'" in w
assert "ID_ET: 'FM-1_2117'" in w
b64=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',w,re.S)
assert b64 and base64.b64decode(b64.group(1))==raw
print('PASS V1.3.2: firmware hash, unchanged OTA, 12 HELP chapters, synchronized WebSimulator, syntax and installer')
PY
