#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
W="$(mktemp -d)"; trap 'rm -rf "$W"' EXIT
C='-O1 -g -std=c11 -fsanitize=undefined -fno-omit-frame-pointer -Ifirmware/fm1-native'
FULL='firmware/fm1-native/track_core.c firmware/fm1-native/track_song.c firmware/fm1-native/track_chip.c firmware/fm1-native/track_drum.c firmware/fm1-native/beetle_drum_rom.c firmware/fm1-native/track_mixer.c firmware/fm1-native/track_project_store.c'
for x in v135_independent_fx v135_fx_pool_atomic v134_block_multi; do
 cc $C "tests/track/test_${x}.c" $FULL -lm -o "$W/$x"
 UBSAN_OPTIONS=halt_on_error=1 "$W/$x"
done
for x in v133_range v133_stems v13_punch_song v13_poly_pan rc1011b_audio_stability v12_usb_src v12_wave_balance; do
 case "$x" in
 v133_range) SRC='firmware/fm1-native/track_core.c';;
 v133_stems|v13_poly_pan) SRC='firmware/fm1-native/track_chip.c firmware/fm1-native/track_drum.c firmware/fm1-native/beetle_drum_rom.c firmware/fm1-native/track_mixer.c';;
 v13_punch_song|rc1011b_audio_stability) SRC='firmware/fm1-native/track_core.c firmware/fm1-native/track_song.c firmware/fm1-native/track_chip.c firmware/fm1-native/track_drum.c firmware/fm1-native/beetle_drum_rom.c firmware/fm1-native/track_mixer.c';;
 v12_wave_balance) SRC='firmware/fm1-native/track_chip.c firmware/fm1-native/track_mixer.c';;
 *) SRC='';;
 esac
 cc $C "tests/track/test_${x}.c" $SRC -lm -o "$W/$x"
 UBSAN_OPTIONS=halt_on_error=1 "$W/$x"
done
cc $C tests/track/test_rc11_help_chapters.c -o "$W/help" && "$W/help"
cc $C tests/track/test_v135_chromatic_key.c firmware/fm1-native/track_core.c -o "$W/chromatic" && "$W/chromatic"
node tests/track/test_v137_web_fx.cjs
python tests/web/test_v135_black_keys.py
python tests/web/test_v135_fx3_ui.py
python tests/web/test_v136_settings_navigation.py
python tests/web/test_v137_all_in_one.py
python tests/track/test_v137_settings_static.py
cc -O2 -std=c11 -Ifirmware/fm1-native tests/track/test_v137_settings_physical.c -o "$W/settings" && "$W/settings"
python - <<'PY'
from pathlib import Path
import re,base64,hashlib,json,sys,subprocess,tempfile
sys.path.insert(0,'tools')
import fm1_fwsc
fw=Path('BEETLE-V1.3.7-USB-MULTI-FX-EXPERIMENTAL.fwsc')
raw=fw.read_bytes();m=json.loads(Path('FIRMWARE-MANIFEST.json').read_text())
assert m['identity']=='FM-1_2122' and m['schema']==22
assert hashlib.sha256(raw).hexdigest()==m['sha256'] and all(m['checks'].values())
report,errors=fm1_fwsc.verify_fwsc(fw)
assert not errors and report['product']=='FM-1_2122'
installer=Path('WebInstaller.html').read_text()
assert "ID_ET: 'FM-1_2122'" in installer and "HASH_ET: '"+m['sha256']+"'" in installer
assert Path('WebInstaller.html').read_bytes()==Path('web/WebInstaller.html').read_bytes()
embedded=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',installer,re.S)
assert embedded and base64.b64decode(embedded[1])==raw
h=Path('firmware/fm1-native/beetle_help.h').read_text()
entries=re.findall(r'"([^"\\]*)"',h.split('static const char *const beetle_help_lines[] = {')[1].split('};')[0]);assert len(entries)>100
p=Path('web/BEETLE-FirmwareSimulator-V1.3.7.html').read_text();lines=re.search(r'const HELP_LINES=(\[.*?\]);',p)
assert lines and entries==json.loads(lines[1])
assert Path('simulator.html').read_bytes()==Path('web/simulator.html').read_bytes()
assert Path('simulator.html').read_bytes()==Path('BEETLE-V1.3.7-Workbench.html').read_bytes()
assert Path('simulator.html').read_bytes()==Path('BEETLE-V1.3.7-simulator.html').read_bytes()
for f in ['simulator.html','web/BEETLE-FirmwareSimulator-V1.3.7.html']:
 s=Path(f).read_text();blocks=re.findall(r'<script(?:\s[^>]*)?>(.*?)</script>',s,re.S)
 with tempfile.NamedTemporaryFile('w',suffix='.js') as out:
  out.write('\n'.join(blocks));out.flush();subprocess.run(['node','--check',out.name],check=True)
print('PASS V1.3.7 FWSC, installer, V22, HELP parity, revised Workbench, JS syntax')
PY
