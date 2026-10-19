#!/usr/bin/env python3
"""Pin the exact V1.3.1 binary to the pre-existing verified installer (no bypass)."""
import base64,hashlib,re,sys
from pathlib import Path
if len(sys.argv)!=4: raise SystemExit('usage: embed_v13_webinstaller.py OLD.html V1.3.1.fwsc NEW.html')
source,fwsc,target=map(Path,sys.argv[1:]); h=source.read_text(); raw=fwsc.read_bytes()
expected=hashlib.sha256(raw).hexdigest()
h=h.replace('BEETLE V1.3','BEETLE V1.3.1').replace('BEETLE-V1.3','BEETLE-V1.3.1')
h,n=re.subn(r"HASH_ET: '[a-f0-9]{64}'","HASH_ET: '"+expected+"'",h,count=1)
assert n==1
h=h.replace("ID_ET: 'FM-1_2115'","ID_ET: 'FM-1_2116'")
h=h.replace('9 DRAWBARS / 3 PARTS','6 LANES / USB AUDIO')
# Do not modify recovery or byte-level identity check implementation.
h,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)([\s\S]*?)(\s*</script>)',lambda m:m.group(1)+base64.b64encode(raw).decode()+m.group(3),h,count=1)
assert n==1
assert "ID_ET: 'FM-1_2116'" in h and "HASH_ET: '"+expected+"'" in h
assert base64.b64encode(raw).decode() in h
target.write_text(h)
print('Installer pinned:',expected,'bytes:',len(raw),'html:',target)
