#!/usr/bin/env python3
"""Source-level regression for 27-key drum audition without tracker mutation."""
from pathlib import Path
base=Path(__file__).resolve().parents[2]
runtime=(base/'firmware/fm1-native/track_runtime.h').read_text()
screen=runtime[runtime.index('static void track_fm1_drum_editor_screen(void)'):runtime.index('/* Pixel-perfect 4-knob MASTER FX')]
assert 'ANY KEY: PREVIEW SELECTED DRUM' in screen
section=runtime[runtime.index('/* V0.80 DRUM ENGINE EDIT:'):runtime.index('if(track_edit_prev_keys){track_audio_all_off',runtime.index('/* V0.80 DRUM ENGINE EDIT:'))]
assert 'if(track_page==16u)' in section
assert 'fm1_input_note_edges()' in section
assert 'k<27u' in section
assert 'track_drum_trigger(&track_drum,track_drum_component)' in section
assert 'fm1_irq_off()' in section and 'fm1_irq_enable_all()' in section
assert 'track_enter_note' not in section and 'track_state.dirty' not in section and 'track_state.active' not in section
assert '(track_state.record_armed && track_page!=16u && track_page!=30u)' in runtime # Live Mixer overrides REC keys
sim=base.parent/'simulator.html'
# During development the simulator may still have the previous staged filename.
if not sim.exists():sim=base.parent/'BEETLE-V0.79-WebSimulator.html'
w=sim.read_text()
assert "model.screen==='drumeditor'" in w
assert 'audio.drumHit(model.drumComponent' in w
assert 'DRUM_PARAMS' in w
assert "DRUM_FACTORY" in w
assert 'NES PULSE' in w
print('PASS V0.80 drum editor preview routing, 27 keys, no tracker entry; simulator editor')
