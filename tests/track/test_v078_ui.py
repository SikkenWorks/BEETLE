#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[2]
a=(r/'firmware/fm1-native/track_runtime.h').read_text()
b=(r/'web/simulator.html').read_text()
h=(r/'firmware/fm1-native/track_chip.h').read_text()
assert '"HELP","CREDITS","DIAGNOSTICS"' in a
assert 'track_menu_index==5u' in a and 'track_page=28u' in a
assert 'if(track_page==28u){' in a
assert 'JOPPE SIKKEN' in a and 'LEO KUROSHITA' in a and 'DAG LEM' in a and 'SHAY GREEN' in a
assert 'NES TECHNICAL REFERENCE' in a and 'NO NES_SND_EMU CODE USED' not in a
assert 'NES TECHNICAL REFERENCE' in b and 'NO NES_SND_EMU CODE USED' not in b
assert "'HELP','CREDITS','DIAGNOSTICS'" in b
assert "model.screen='credits'" in b and 'function creditsScreen()' in b
assert all(x in b for x in ['NES PULSE','NES TRIANGLE','NES NOISE'])
assert all(x in h for x in ['TRACK_CHIP_NES_PULSE 9u','TRACK_CHIP_NES_TRIANGLE 10u','TRACK_CHIP_NES_NOISE 11u','TRACK_CHIP_TYPES 12u'])
print('PASS V0.78 native/browser CREDITS, 3 NES types, original IDs retained')
