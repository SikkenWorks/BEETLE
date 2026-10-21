#!/usr/bin/env python3
"""R1-specific release checks: no physical FM1 interaction."""
from pathlib import Path
import sys,re,base64,hashlib,json
R=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'tools'))
import fm1_fwsc as fw
ref=(R/'reference/BEETLE-V1.3.8-ORIGINAL-ROLLBACK.fwsc').read_bytes()
new=(R/'BEETLE-V1.3.8-R1-SUSTAIN-FIX-EXPERIMENTAL.fwsc').read_bytes()
meta=json.loads((R/'FIRMWARE-MANIFEST.json').read_text())
assert hashlib.sha256(ref).hexdigest()=='0d5aea5d0f76bff15f0c441b03d7fd6e00503fed5a05b1bc308eff8b73bfc485'
assert hashlib.sha256(new).hexdigest()==meta['sha256']
assert meta['identity']=='FM-1_2123' and all(meta['checks'].values())
assert not fw.verify_fwsc(R/'BEETLE-V1.3.8-R1-SUSTAIN-FIX-EXPERIMENTAL.fwsc')[1]
assert not (R/'resid-embedded').exists() and not (R/'resid-port').exists() and not (R/'resid-host').exists()
assert 'beetle_sid_lite.h' in (R/'firmware/fm1-native/track_chip.c').read_text()
s=(R/'firmware/fm1-native/track_runtime.h').read_text()
assert '"NES NOISE"' in s and '"YM2612"' not in s
web=(R/'WebInstaller.html').read_text()
emb=re.search(r'<script id="firmware-embedded" type="application/octet-stream">(.*?)</script>',web,re.S)
assert emb and base64.b64decode(emb.group(1).strip())==new
assert "HASH_ET: '"+meta['sha256']+"'" in web
assert "ID_ET: 'FM-1_2123'" in web
manager=(R/'BEETLE-MANAGER.html').read_text()
for k,file in [('emulator','simulator.html'),('projectmanager','BEETLE-Project-Manager.html'),('installer','WebInstaller.html')]:
 m=re.search(r'<script id="source-'+k+'" type="application/octet-stream">(.*?)</script>',manager,re.S)
 assert m and base64.b64decode(m.group(1).strip())==(R/file).read_bytes(),k
print('PASS R1 source, 12-oscillator baseline, FWSC CRC, original rollback, installer and manager consistency')
