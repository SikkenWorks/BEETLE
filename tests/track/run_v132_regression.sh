#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
work="$(mktemp -d)";trap 'rm -rf "$work"' EXIT
cc -O1 -g -std=c11 -fsanitize=undefined -fno-omit-frame-pointer -Ifirmware/fm1-native \
 tests/track/test_v132_instruments_octave.c \
 firmware/fm1-native/{track_core,track_chip,track_song,track_mixer,track_drum,beetle_drum_rom}.c -o "$work/octave"
UBSAN_OPTIONS=halt_on_error=1 "$work/octave"
cc -O2 -std=c11 -Ifirmware/fm1-native tests/track/test_rc1011b_pcm_compat.c \
 firmware/fm1-native/{track_chip,track_mixer}.c -o "$work/pcm"
"$work/pcm"
cc -O2 -std=c11 -Ifirmware/fm1-native tests/track/test_rc11_help_chapters.c -o "$work/help"
"$work/help"
python3 - <<'PY'
from pathlib import Path
import json,re,subprocess,tempfile
h=Path('firmware/fm1-native/beetle_help.h').read_text()
entries=re.findall(r'"([^"\\]*)"',h.split('static const char *const beetle_help_lines[] = {')[1].split('};')[0])
for name in ('simulator.html','web/simulator.html'):
 src=Path(name).read_text()
 match=re.search(r'const HELP_LINES=(\[.*?\]);',src)
 assert match and json.loads(match.group(1))==entries
 assert 'function transposeTrackerOctave(delta)' in src
 assert 'focusInstrumentAtCursor();' in src
 assert 'model.rangeSelection=[model.row,row]' in src
 assert 'model.project.instruments[id]=JSON.parse(JSON.stringify(model.project.instruments[old]))' not in src
 with tempfile.NamedTemporaryFile('w',suffix='.js') as f:
  f.write('\n'.join(re.findall(r'<script(?:\s[^>]*)?>(.*?)</script>',src,re.S)))
  f.flush();subprocess.run(['node','--check',f.name],check=True)
runtime=Path('firmware/fm1-native/track_runtime.h').read_text()
assert 'track_edit_focus_step(track_state.transport.edit_channel)' in runtime
assert 'track_transpose_note_range(&track_state,ch,a,b,octave_delta)' in runtime
assert 'track_transpose_note_range(&track_state,ch,a,b,semitones)' in runtime
assert 'track_library_apply_active(id,&copy)' not in runtime
assert any('SEL+K1 RANGE' in x for x in entries)
assert any('DEFAULT 7 SID' in x for x in entries)
print('PASS V1.3.2: native+web patch focus, direct range octave, no auto-clone, 12 synced HELP chapters, valid simulator JS')
PY
