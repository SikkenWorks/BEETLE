#!/usr/bin/env python3
"""Embed the exact V1.3.2 experimental .fwsc in the existing installer.
Do not change WebMIDI OTA/restore policy or unrelated original ETCH identities.
"""
from pathlib import Path
import hashlib,base64,re,sys
if len(sys.argv)!=4:raise SystemExit('usage: embed_v132_webinstaller.py base.html new.fwsc output.html')
base,fwsc,out=map(Path,sys.argv[1:]);src=base.read_text();binary=fwsc.read_bytes()
assert src.count("ID_ET: 'FM-1_2116'")==1
assert src.count('id="firmware-embedded"')==1
sha=hashlib.sha256(binary).hexdigest()
src=src.replace('BEETLE V1.3.1','BEETLE V1.3.2').replace('BEETLE-V1.3.1','BEETLE-V1.3.2')
src=src.replace("ID_ET: 'FM-1_2116'","ID_ET: 'FM-1_2117'")
src,n=re.subn(r"HASH_ET: '[a-f0-9]{64}'","HASH_ET: '"+sha+"'",src,count=1);assert n==1
src,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)(.*?)(\s*</script>)',lambda m:m[1]+base64.b64encode(binary).decode('ascii')+m[3],src,count=1,flags=re.S);assert n==1
assert "ID_ET: 'FM-1_2117'" in src and "HASH_ET: '"+sha+"'" in src
out.write_text(src)
print('PASS installer synchronized',sha)
