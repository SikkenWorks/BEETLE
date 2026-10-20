#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
WORK="$(mktemp -d)"; trap 'rm -rf "$WORK"' EXIT
C='-O1 -g -std=c11 -fsanitize=undefined -fno-omit-frame-pointer -Ifirmware/fm1-native'
cc $C tests/track/test_v134_block_multi.c firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer,track_project_store}.c -lm -o "$WORK/blockmulti"
UBSAN_OPTIONS=halt_on_error=1 "$WORK/blockmulti"
cc $C tests/track/test_v134_multi_fx.c firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer,track_project_store}.c -lm -o "$WORK/multi"
UBSAN_OPTIONS=halt_on_error=1 "$WORK/multi"
cc $C tests/track/test_v134_patch_slots.c firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer,track_project_store}.c -o "$WORK/patch"
UBSAN_OPTIONS=halt_on_error=1 "$WORK/patch" tests/fixtures/BEETLE-V19-real-legacy-project.trk "$WORK/V21.trk"
node tests/track/test_v134_web_codec.cjs tests/fixtures/BEETLE-V19-real-legacy-project.trk "$WORK/V21.trk"
cc $C tests/track/test_rc11_help_chapters.c -o "$WORK/help"; "$WORK/help"
for test in v133_range v133_stems v13_punch_song v13_poly_pan rc1011b_audio_stability v12_usb_src v12_wave_balance; do
 file="tests/track/test_${test}.c"
 case "$test" in
  v133_range) files='firmware/fm1-native/track_core.c';;
  v133_stems|v13_poly_pan) files='firmware/fm1-native/track_chip.c firmware/fm1-native/track_drum.c firmware/fm1-native/beetle_drum_rom.c firmware/fm1-native/track_mixer.c';;
  v13_punch_song|rc1011b_audio_stability) files='firmware/fm1-native/track_core.c firmware/fm1-native/track_song.c firmware/fm1-native/track_chip.c firmware/fm1-native/track_drum.c firmware/fm1-native/beetle_drum_rom.c firmware/fm1-native/track_mixer.c';;
  v12_wave_balance) files='firmware/fm1-native/track_chip.c firmware/fm1-native/track_mixer.c';;
  *) files='';;
 esac
 cc $C "$file" $files -lm -o "$WORK/$test"
 UBSAN_OPTIONS=halt_on_error=1 "$WORK/$test"
done
python - <<'PY'
from pathlib import Path
import base64,hashlib,json,re,sys,subprocess,tempfile
sys.path.insert(0,'tools');import fm1_fwsc
mf=json.loads(Path('FIRMWARE-MANIFEST.json').read_text())
f=Path('BEETLE-V1.3.4-USB-MULTI-FX-EXPERIMENTAL.fwsc');raw=f.read_bytes()
assert mf['identity']=='FM-1_2119' and mf['schema']==21 and mf['extra_fx_pool_cap']==48
assert mf['sha256']==hashlib.sha256(raw).hexdigest() and all(mf['checks'].values())
report,errs=fm1_fwsc.verify_fwsc(f)
assert not errs and report['product']=='FM-1_2119'
installer=Path('WebInstaller.html').read_text()
assert "ID_ET: 'FM-1_2119'" in installer and "HASH_ET: '"+mf['sha256']+"'" in installer
embed=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',installer,re.S)
assert embed and base64.b64decode(embed[1])==raw
assert Path('web/WebInstaller.html').read_bytes()==Path('WebInstaller.html').read_bytes()
for f in ('simulator.html','web/simulator.html'):
 s=Path(f).read_text();lines=re.search(r'const HELP_LINES=(\[.*?\]);',s)
 assert lines
 import re
 native=Path('firmware/fm1-native/beetle_help.h').read_text()
 entries=re.findall(r'"([^"\\]*)"',native.split('static const char *const beetle_help_lines[] = {')[1].split('};')[0]);assert entries==json.loads(lines[1])
 assert 'FX STORAGE FULL' in s and 'fxPoolCount' in s and 'extraFx' in s and "write16(4,21)" in s
 matches=re.findall(r'<script(?:\s[^>]*)?>(.*?)</script>',s,re.S)
 with tempfile.NamedTemporaryFile(mode='w',suffix='.js') as tmp:
  tmp.write('\n'.join(matches));tmp.flush();subprocess.run(['node','--check',tmp.name],check=True)
assert Path('firmware/fm1-native/track_project_store.h').read_text().count('TRACK_STORE_VERSION 21u')==1
print('PASS V1.3.4: validated package header, loader, six-channel-compatible firmware, installer SHA256/embed, V21, HELP, browser scripts')
PY
