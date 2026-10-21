#!/usr/bin/env python3
"""Re-embed verified V0.52 FM1 .fwsc in the existing audited OTA installer.
No hardware or USB access. Never expands the update allowlist beyond known IDs.
"""
import sys,re,base64,hashlib
from pathlib import Path
if len(sys.argv)!=4: raise SystemExit('usage: embed_v052_webinstaller.py v051.html v052.fwsc v052.html')
source,fw,out=map(Path,sys.argv[1:]);data=fw.read_bytes();h=source.read_text()
if len(data)!=610086:raise SystemExit('Unexpected firmware size')
if "ID_ET: 'FM-1_2051'" not in h:raise SystemExit('v051 base identity mismatch')
h=h.replace('V0.51','V0.52').replace('BEETLE-0.51','BEETLE-0.52')
h=h.replace("ID_ET: 'FM-1_2051'","ID_ET: 'FM-1_2052'")
pre="'FM-1_2049','FM-1_2050','FM-1_2051'"
if pre not in h:raise SystemExit('Predecessor firmware IDs not recognized')
h=h.replace(pre,pre+",'FM-1_2052'")
h,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'","HASH_ET: '"+hashlib.sha256(data).hexdigest()+"'",h,count=1)
if n!=1:raise SystemExit('Missing firmware hash anchor')
h,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)([\s\S]*?)(\s*</script>)',lambda m:m.group(1)+base64.b64encode(data).decode()+m.group(3),h,count=1)
if n!=1:raise SystemExit('Missing embedded firmware anchor')
if 'FM1 DIGITAL DRUMS' in h:raise SystemExit('Stale product branding')
if 'FM-1_2052' not in h:raise SystemExit('Stale version')
if "document.addEventListener('contextmenu'" not in h:
 h=h.replace('</body>',"<script>document.addEventListener('contextmenu',e=>e.preventDefault());document.addEventListener('selectstart',e=>e.preventDefault());</script></body>")
out.write_text(h)
print('Embedded BEETLE V0.52',out,'SHA256',hashlib.sha256(data).hexdigest())
