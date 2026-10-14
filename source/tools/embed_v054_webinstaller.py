#!/usr/bin/env python3
"""Re-embed verified V0.54 FM1 .fwsc in the existing audited OTA installer.
No hardware or USB access. Never expands the update allowlist beyond known IDs.
"""
import sys,re,base64,hashlib
from pathlib import Path
if len(sys.argv)!=4: raise SystemExit('usage: embed_v054_webinstaller.py v053.html v054.fwsc v054.html')
source,fw,out=map(Path,sys.argv[1:]);data=fw.read_bytes();h=source.read_text()
if len(data)<500000 or len(data)>720000:raise SystemExit('Unexpected firmware size')
if "ID_ET: 'FM-1_2053'" not in h:raise SystemExit('v052 base identity mismatch')
h=h.replace('V0.53','V0.54').replace('BEETLE-0.53','BEETLE-0.54')
h=h.replace("ID_ET: 'FM-1_2053'","ID_ET: 'FM-1_2054'")
pre="'FM-1_2050','FM-1_2051','FM-1_2052','FM-1_2053'"
if pre not in h:raise SystemExit('Predecessor firmware IDs not recognized')
h=h.replace(pre,pre+",'FM-1_2054'")
h,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'","HASH_ET: '"+hashlib.sha256(data).hexdigest()+"'",h,count=1)
if n!=1:raise SystemExit('Missing firmware hash anchor')
h,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)([\s\S]*?)(\s*</script>)',lambda m:m.group(1)+base64.b64encode(data).decode()+m.group(3),h,count=1)
if n!=1:raise SystemExit('Missing embedded firmware anchor')
if 'FM1 DIGITAL DRUMS' in h:raise SystemExit('Stale product branding')
if 'FM-1_2054' not in h:raise SystemExit('Stale version')
if "document.addEventListener('contextmenu'" not in h:
 h=h.replace('</body>',"<script>document.addEventListener('contextmenu',e=>e.preventDefault());document.addEventListener('selectstart',e=>e.preventDefault());</script></body>")
out.write_text(h)
print('Embedded BEETLE V0.54',out,'SHA256',hashlib.sha256(data).hexdigest())
