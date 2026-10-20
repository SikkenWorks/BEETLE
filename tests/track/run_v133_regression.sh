#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
C='-O1 -g -std=c11 -fsanitize=undefined -fno-omit-frame-pointer -Ifirmware/fm1-native'
cc $C tests/track/test_v133_range.c firmware/fm1-native/track_core.c -o "$work/range"
UBSAN_OPTIONS=halt_on_error=1 "$work/range"
cc $C tests/track/test_v133_stems.c firmware/fm1-native/{track_chip,track_drum,beetle_drum_rom,track_mixer}.c -o "$work/stems"
UBSAN_OPTIONS=halt_on_error=1 "$work/stems"
cc $C tests/track/test_v131_patch_slots.c firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer,track_project_store}.c -o "$work/patch"
UBSAN_OPTIONS=halt_on_error=1 "$work/patch" tests/fixtures/BEETLE-V19-real-legacy-project.trk "$work/latest-v20.trk"
node tests/track/test_v131_web_codec.cjs tests/fixtures/BEETLE-V19-real-legacy-project.trk "$work/latest-v20.trk"
cc $C tests/track/test_v13_punch_song.c firmware/fm1-native/{track_song,track_core,track_drum,beetle_drum_rom,track_chip,track_mixer}.c -lm -o "$work/punch"
UBSAN_OPTIONS=halt_on_error=1 "$work/punch"
cc $C tests/track/test_v13_poly_pan.c firmware/fm1-native/{track_chip,track_mixer,track_drum,beetle_drum_rom}.c -lm -o "$work/pan"
UBSAN_OPTIONS=halt_on_error=1 "$work/pan"
cc $C tests/track/test_rc1011b_audio_stability.c firmware/fm1-native/{track_core,track_song,track_chip,track_drum,beetle_drum_rom,track_mixer}.c -o "$work/song"
UBSAN_OPTIONS=halt_on_error=1 "$work/song"
cc $C tests/track/test_v12_usb_src.c -lm -o "$work/usb"
"$work/usb"
cc $C tests/track/test_v12_wave_balance.c firmware/fm1-native/{track_chip,track_mixer}.c -lm -o "$work/wave"
"$work/wave"
cc $C tests/track/test_rc1011b_pcm_compat.c firmware/fm1-native/{track_chip,track_mixer}.c -o "$work/pcm"
[ "$("$work/pcm")" = 'PCM FNV1A32 fbda64f6' ]
cc $C tests/track/test_rc11_help_chapters.c -o "$work/help"
"$work/help"
python3 - <<'PY'
from pathlib import Path
import base64,hashlib,json,re,subprocess,tempfile,sys
sys.path.insert(0,'tools')
import fm1_fwsc
fm=Path('BEETLE-V1.3.3-USB-MULTITRACK-EXPERIMENTAL.fwsc').read_bytes()
m=json.loads(Path('FIRMWARE-MANIFEST.json').read_text())
assert m['identity']=='FM-1_2118' and m['schema']==20
assert hashlib.sha256(fm).hexdigest()==m['sha256']
assert all(m['checks'].values())
parsed,errors=fm1_fwsc.verify_fwsc(Path('BEETLE-V1.3.3-USB-MULTITRACK-EXPERIMENTAL.fwsc'))
assert not errors and parsed['product']=='FM-1_2118'
ins=Path('WebInstaller.html').read_text()
assert "HASH_ET: '"+m['sha256']+"'" in ins
assert "ID_ET: 'FM-1_2118'" in ins
match=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',ins,re.S)
assert match and base64.b64decode(match.group(1))==fm
assert Path('web/WebInstaller.html').read_bytes()==Path('WebInstaller.html').read_bytes()
h=Path('firmware/fm1-native/beetle_help.h').read_text()
entries=re.findall(r'"([^"\\]*)"',h.split('static const char *const beetle_help_lines[] = {')[1].split('};')[0])
assert sum(x.startswith('# ') for x in entries)==13
assert 'USB MULTITRACK' in ' '.join(entries)
for fn in ('simulator.html','web/simulator.html'):
 s=Path(fn).read_text();a=re.search(r'const HELP_LINES=(\[.*?\]);',s)
 assert a and json.loads(a.group(1))==entries
 assert 'usbSettingsScreen()' in s and 'model.rangeSelection' in s and 'SELECTION CLEARED' in s
 scripts='\n'.join(re.findall(r'<script(?:\s[^>]*)?>(.*?)</script>',s,re.S))
 with tempfile.NamedTemporaryFile('w',suffix='.js') as f:
  f.write(scripts);f.flush();subprocess.run(['node','--check',f.name],check=True)
source=Path('firmware/fm1-native/usb.c').read_text()
assert 'D_FMT_MT 11, 0x24, 2, 1, BEETLE_USB_STEMS, 2, 16, 1' in source
assert 'uac_tap_stems' in source and 'uac_packet_stems' in source
assert 'uac_multitrack?UAC_MT_MAXP' not in source # 8bit SIE register must not receive byte count
assert 'sie_wr(S_TXMAXP, 0xFF)' in source
rt=Path('firmware/fm1-native/track_runtime.h').read_text()
assert 'usb_multitrack_switch' in rt and 'track_set_instrument_range' in rt and 'track_clear_step_range' in rt
print('PASS V1.3.3: V20 codec, 13 HELP chapters, six-channel USB descriptor and native taps, mode switch, synced installer hash, syntax')
PY
