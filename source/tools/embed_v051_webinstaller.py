#!/usr/bin/env python3
"""Embed V0.51 audited firmware in V0.50 OTA installer. No device access."""
import sys,re,base64,hashlib
from pathlib import Path
if len(sys.argv)!=4: raise SystemExit('usage: embed_v051_webinstaller.py old.html firmware.fwsc out.html')
source,fw,out=map(Path,sys.argv[1:]);data=fw.read_bytes();h=source.read_text()
if len(data)!=610086:raise SystemExit('unexpected .fwsc size')
if "ID_ET: 'FM-1_2050'" not in h:raise SystemExit('base installer wrong identity')
h=h.replace('V0.50','V0.51').replace("ID_ET: 'FM-1_2050'","ID_ET: 'FM-1_2051'")
# Add new version to accepted known predecessor IDs while preserving all old firmware IDs.
needle="'FM-1_2047','FM-1_2048','FM-1_2049','FM-1_2050'"
if needle not in h:raise SystemExit('predecessor list missing')
h=h.replace(needle,needle+",'FM-1_2051'")
h,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'","HASH_ET: '"+hashlib.sha256(data).hexdigest()+"'",h,count=1)
if n!=1:raise SystemExit('hash replacement failure')
h,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)([\s\S]*?)(\s*</script>)',lambda m:m.group(1)+base64.b64encode(data).decode()+m.group(3),h,count=1)
if n!=1:raise SystemExit('firmware embedding failure')
if "ID_ET: 'FM-1_2051'" not in h:raise SystemExit('ID mismatch')
h=h.replace('</body>',"<script>document.addEventListener('contextmenu',e=>e.preventDefault());document.addEventListener('selectstart',e=>e.preventDefault());</script></body>")
out.write_text(h)
print('V0.51 installer',out,'firmware sha',hashlib.sha256(data).hexdigest())
