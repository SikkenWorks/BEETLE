#!/usr/bin/env python3
"""Re-embed audited firmware without touching protected boot or OTA bytes."""
import base64,hashlib,pathlib,re,sys
if len(sys.argv)!=4:sys.exit('usage: embed_v045_webinstaller.py previous.html firmware.fwsc output.html')
source,fw,out=map(pathlib.Path,sys.argv[1:]);h=source.read_text();b=fw.read_bytes()
if len(b)!=610086:sys.exit('unexpected firmware package length')
h=h.replace('V0.44','V0.45').replace('FM-1_2044','FM-1_2045')
# Restore predecessor identity explicitly; do not replace existing allowlist entry.
needle="'FM-1_2043','FM-1_2045'"
if needle not in h:sys.exit('expected last allowlist entries missing')
h=h.replace(needle,"'FM-1_2043','FM-1_2044','FM-1_2045'")
h,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'", "HASH_ET: '"+hashlib.sha256(b).hexdigest()+"'",h,count=1)
if n!=1:sys.exit('expected immutable ET hash missing')
h,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)([\s\S]*?)(\s*</script>)',lambda m:m.group(1)+base64.b64encode(b).decode()+m.group(3),h,count=1)
if n!=1:sys.exit('embedded fwsc script missing')
out.write_text(h)
print('webinstaller',out,'sha256',hashlib.sha256(b).hexdigest())
