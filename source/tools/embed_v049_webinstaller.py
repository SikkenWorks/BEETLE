#!/usr/bin/env python3
"""Embed V0.49 lab artifact into the established V0.48 installer UX.
Immutable boot+OTA hashes are checked by the fwsc packer, not rewritten here.
"""
import base64,hashlib,pathlib,re,sys
if len(sys.argv)!=4:sys.exit('usage: embed_v049_webinstaller.py v048.html v049.fwsc output.html')
source,fw,out=map(pathlib.Path,sys.argv[1:]);h=source.read_text();b=fw.read_bytes()
if len(b)!=610086:sys.exit('unexpected firmware package length')
h=h.replace('V0.48','V0.49').replace("ID_ET: 'FM-1_2048'","ID_ET: 'FM-1_2049'")
needle="'FM-1_2045','FM-1_2046','FM-1_2047','FM-1_2048'"
if needle not in h:sys.exit('expected predecessor allowlist entries missing')
h=h.replace(needle,"'FM-1_2045','FM-1_2046','FM-1_2047','FM-1_2048','FM-1_2049'")
h,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'", "HASH_ET: '"+hashlib.sha256(b).hexdigest()+"'",h,count=1)
if n!=1:sys.exit('expected firmware hash field missing')
h,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)([\s\S]*?)(\s*</script>)',lambda m:m.group(1)+base64.b64encode(b).decode()+m.group(3),h,count=1)
if n!=1:sys.exit('embedded fwsc script missing')
h=h.replace('SIKKENWORKS · V0.38 · EXPERIMENTAL','SIKKENWORKS · V0.49 · EXPERIMENTAL')
h=h.replace('with a rebuilt 14-sound digital drum synthesizer','with faster project browsing, corrected DISCARD navigation and automatic return to the tracker after saving')
h=h.replace('</body>',"<script>document.addEventListener('contextmenu',e=>e.preventDefault());document.addEventListener('selectstart',e=>e.preventDefault());</script></body>")
out.write_text(h)
print('webinstaller',out,'sha256',hashlib.sha256(b).hexdigest())
