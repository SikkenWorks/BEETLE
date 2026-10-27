#!/usr/bin/env python3
"""Mandatory current-device HELP / browser HELP consistency regression."""
from pathlib import Path
import json,re
root=Path(__file__).resolve().parents[2]
header=(root/'firmware/fm1-native/beetle_help.h').read_text(encoding='utf-8')
lines=re.findall(r'^\s*"([^"\\]*)"',header.split('static const char *const beetle_help_lines[] = {',1)[1].split('};',1)[0],re.M)
assert 1<len(lines)<256,'Native HELP index is uint8_t; cannot exceed 255 lines'
assert all(len(x)<=36 for x in lines), [(len(x),x) for x in lines if len(x)>36]
assert lines[0].startswith('#')
chapters=[(i,x) for i,x in enumerate(lines) if x.startswith('#')]
assert len(chapters)>=13,'Lost existing HELP chapter(s)'
for a,b in zip(chapters,chapters[1:]):
 assert b[0]>a[0]+1,('Empty HELP chapter',a)
html=(root/'simulator.html').read_text(encoding='utf-8')
match=re.search(r'const HELP_LINES=(\[.*?\]);',html,re.S)
assert match,'Simulator HELP_LINES missing'
web_lines=json.loads(match[1]);assert lines==web_lines,'Native firmware and simulator HELP mismatch'
joined='\n'.join(lines)
for needle in (
 'CH6 SEL+FX: FX / NOTE3 VIEW',
 'CH6 NOTE3: K2 FOCUS K3 SELECT',
 'CH6 K3: PREVIEW NOTE/NOTE2/NOTE3',
 'N3: 14 DRUMS + U01-U13 SAMPLES',
 'N3: --- EMPTY, STP STOPS ONLY N3',
 'N3: SEL DELETES ONLY NOTE3',
 'N3 ON: CH6 STEP FX NOT PLAYED',
 'FX RESTORED WHEN N3 TURNED OFF',
 'WRITING NOTE KEEPS CURRENT STEP',
 'SAVE ALSO STORES NOTE3 ON/OFF',
 'LOAD RESTORES NOTE3 MODE + HITS',
 'LOAD: EDIT+SEL DELETE PROJECT',
 'DELETE: K1 YES/NO, SEL CONFIRM',
 'DELETE: HOME CANCEL, NO DEFAULT',
 'HOLD SEL 650MS: SONG LOOP ON/OFF',
 'SEL+K1: MARK SONG ROW RANGE',
 'PLAY: LOOP ONLY SELECTED ROWS',
 'NO RANGE: USE SONG LOOP ON/OFF',
):assert needle in joined,('Missing/on-device HELP outdated',needle)
assert 'SEL: SONG ON/OFF' not in lines,'Obsolete song SEL behavior'
assert 'SEL: CLEAR SELECTED STEP' not in lines,'Obsolete note deletion instruction'
assert 'SYNTH NOTES IN RANGE (EXCEPT ---/STP)' not in lines,'Overlong legacy HELP string'
readme=(root/'README.md').read_text()
assert not re.search(r'^#{1,6}\s*(?:New in\s+V\d|V\d+(?:\.\d+){2,}\s*(?:—|[-:]))',readme,re.I|re.M)
print('PASS: HELP parity,',len(lines),'native/browser lines,',len(chapters),'chapters, 36-character width and NOTE3/SONG controls')
