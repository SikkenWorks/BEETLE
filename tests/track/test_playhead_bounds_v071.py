#!/usr/bin/env python3
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
html=(root/'TRACK-WebSimulator-V0.7.1.html').read_text()
native=(root/'firmware/fm1-native/track_runtime.h').read_text()
# The first field is center-aligned around X=36. For a four-character
# monospace token '[00]' at 8 px, the opening bracket starts < 27px.
# The highlight must begin well before it (<= 21px), and end after FX. 
assert "rect(21,y-2,201,13,'#f500c7')" in html
assert "rect(23,y,197,9,'#8d006f')" in html
assert "if(model.row===r&&!model.playing)rect(21,y-1,201,11" in html
assert "vals.forEach((v,k)=>text('['+v+']'" in html
# Native LCD: first bracket at 35 and last closes at 210; both inside 29..213.
assert "static const uint16_t bx[]={35u,71u,107u,143u,179u}" in native
assert 'lcd_fill(29u,y-2u,184u,11u' in native
assert 'lcd_fill(29u,y-1u,184u,10u' in native
assert native.index('lcd_fill(29u,y-2u,184u,11u') < native.index('for(uint8_t j=0u;j<5u;j++)track_bracket(')
print('TRACK 0.7.1: bracket-inclusive playback and idle row bounds PASS')
