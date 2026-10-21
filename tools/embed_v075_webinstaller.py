#!/usr/bin/env python3
"""Embed the matching audited 0.75 .fwsc; no hardware writes."""
import base64,hashlib,re,sys
from pathlib import Path
if len(sys.argv)!=4: raise SystemExit('usage: embed_v075_webinstaller.py template.html firmware.fwsc output.html')
base,firmware,output=map(Path,sys.argv[1:]); data=firmware.read_bytes();h=base.read_text()
if not 500000<=len(data)<=720000:raise SystemExit('Unexpected binary size')
if "ID_ET: 'FM-1_2075'" not in h or "'FM-1_2061','FM-1_2062','FM-1_2063','FM-1_2064','FM-1_2065','FM-1_2066','FM-1_2067','FM-1_2068','FM-1_2069','FM-1_2070','FM-1_2071','FM-1_2072','FM-1_2073','FM-1_2074','FM-1_2075'" not in h:raise SystemExit('Installer ID or predecessor allowlist mismatch')
for expression,replacement in [
 (r'ET_LENGTH: \d+',f'ET_LENGTH: {len(data)}'),
 (r"HASH_ET: '[0-9a-f]{64}'",f"HASH_ET: '{hashlib.sha256(data).hexdigest()}'"),
]:
 h,n=re.subn(expression,replacement,h,count=1)
 if n!=1:raise SystemExit('Anchor missing '+expression)
h,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)([\s\S]*?)(\s*</script>)',lambda m:m.group(1)+base64.b64encode(data).decode()+m.group(3),h,count=1)
if n!=1:raise SystemExit('Embedded firmware block missing')
if 'FM1 DIGITAL DRUMS' in h:raise SystemExit('Old branding')
output.write_text(h)
print(hashlib.sha256(data).hexdigest(),len(data))
