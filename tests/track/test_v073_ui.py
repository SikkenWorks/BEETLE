#!/usr/bin/env python3
"""Source-level UI safety assertions; not a hardware GUI test."""
from pathlib import Path
r=Path(__file__).resolve().parents[2]
t=(r/'firmware/fm1-native/track_runtime.h').read_text()
a=(r/'firmware/fm1-native/audio_target.c').read_text()
assert any(f'track_page>{n}u' in t for n in (27,28,29,30))
assert '"DIAGNOSTICS"' in t and ('track_menu_index==5u' in t or 'track_menu_index==6u' in t)
assert 'if(track_page==27u){' in t
assert 'fm1_irq_off();snap=native_rc;' in t
assert 'K1 TAB  HOME MENU  SEQ EXIT' in t
assert t.index('if(track_page==27u){',t.index('/* RC hardware telemetry is read-only')) < t.index('/* HELP never changes patterns')
assert 'beetle_rc_audio(&native_rc,measure)' in a
assert 'beetle_rc_guard(&native_rc)' in a
assert 'beetle_rc_midi(&native_rc' in a
print('PASS V0.73 diagnostics page and ISR instrumentation source assertions')
