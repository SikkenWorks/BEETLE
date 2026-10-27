#!/usr/bin/env python3
"""Source-level regression: native K3 auditory route and physical delete keys."""
from pathlib import Path
r=Path(__file__).resolve().parents[2]
ui=(r/'firmware/fm1-native/track_runtime.h').read_text()
modal=(r/'firmware/fm1-native/beetle_project_delete_ui.h').read_text()
a=ui.index('const uint8_t audition=(uint8_t)')
b=ui.index('   if(preset && (track_state',a)
preview=ui[a:b]
assert 'col==1u' in preview and 'track_note2_id(dr)' in preview
assert 'track_drum_note3_enabled && col==2u' in preview
assert 'track_drum_trigger(&track_drum,drum_id)' in preview
assert 'if(drum_id<TRACK_DRUM_NOTE_COUNT)' in preview
assert 'beetle_load_select_confirm(&track_load_sel_gate' in ui
assert 'if((held|pressed|released)&~BEETLE_CONFIRM_SEL)' in modal
assert 'BEETLE_DELETE_EDIT' in modal and 'BEETLE_CONFIRM_SEL' in modal
assert '(1u<<7)' in modal
assert 'BEETLE_DELETE_SEL' not in modal
assert 'EDIT+SEL' in ui and 'EDIT+GLO' not in ui
print('PASS: physical EDIT+SEL modal; NOTE2/NOTE3 K3 audition uses full drum sample IDs')
