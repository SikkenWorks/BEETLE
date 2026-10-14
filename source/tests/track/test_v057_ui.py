#!/usr/bin/env python3
"""V0.57 lightweight regression for native UI page guard and truthful HELP shortcuts."""
from pathlib import Path
p=Path(__file__).resolve().parents[2]/'firmware'/'fm1-native'
r=(p/'track_runtime.h').read_text();h=(p/'beetle_help.h').read_text()
assert 'if(track_page>20u)' in r, 'Block Editor is page 20'
assert 'if(track_page==20u)' in r
for label in ('SEQ+OP1: COPY ONE LANE','SEQ+OP2: PASTE ONE LANE','SEQ+OP3: COPY WHOLE PATTERN','SEQ+OP4: PASTE PATTERN + PUNCH FX'):
    assert label in h,label
assert 'STEPS 01-16' in r
assert 'track_pix_text("HOME / SEQ: TRACKER",12u,213u' in r
print('PASS V0.57 native Block Editor guard, visual strip and HELP')
