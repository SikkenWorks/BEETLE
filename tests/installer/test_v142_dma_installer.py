#!/usr/bin/env python3
"""Offline integrity, root/web parity, original-loader safety checks."""
from pathlib import Path
import base64,hashlib,re,sys
R=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'tools'))
import fm1_fwsc as fw
html=(R/'WebInstaller.html').read_text()
assert (R/'web/WebInstaller.html').read_bytes()==(R/'WebInstaller.html').read_bytes()
m=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',html,re.S)
assert m,"No embedded file"
payload=base64.b64decode(m.group(1),validate=True)
candidate=(R/'BEETLE-V1.4.5-PLAYHEAD-GHOST-FIX-UNVERIFIED.fwsc').read_bytes()
assert payload==candidate,"Embedded image is not the shipped firmware"
actual=hashlib.sha256(candidate).hexdigest()
assert f"HASH_ET: '{actual}'" in html,"Installer PIN is not this candidate"
assert "ID_ET: 'FM-1_2123'" in html
assert f'ET_LENGTH: {len(candidate)}' in html
assert 'BEETLE V1.3.3' not in html
assert 'V1.4.5 GHOST FIX' in html
obj,errs=fw.verify_fwsc(R/'BEETLE-V1.4.5-PLAYHEAD-GHOST-FIX-UNVERIFIED.fwsc')
assert not errs,errs
assert obj['product']=='FM-1_2123' and obj['bytes']==len(candidate)
ref=fw.parse_ufw((R/'BEETLE-V1.4.1-BASELINE-UNCHANGED.fwsc').read_bytes())
cur=fw.parse_ufw(candidate)
get=lambda obj,name:next(x['data'] for x in obj['entries'] if x['name']==name)
assert get(ref,'flash.bin')[:0x4000]==get(cur,'flash.bin')[:0x4000]
assert get(ref,'ota.bin')==get(cur,'ota.bin')
assert '02 / INSTALL GHOST FIX (UNVERIFIED)' in html
print('PASS: embedded FWSC equals supplied candidate, exact SHA-256, identity, length, CRC, protected loader, parity')
