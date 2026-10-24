#!/usr/bin/env python3
"""Finalize SID regression correction offline, never communicate with hardware."""
from pathlib import Path
import base64,hashlib,json,re
r=Path(__file__).resolve().parents[1]
fw=r/'BEETLE-V1.4.31-SID-WAH-REGRESSION-EXPERIMENTAL.fwsc'
blob=fw.read_bytes();sha=hashlib.sha256(blob).hexdigest()
for name in ('BEETLE-Project-Manager.html','web/BEETLE-Project-Manager.html','simulator.html','web/simulator.html'):
 p=r/name;t=p.read_text().replace('V1.4.30','V1.4.31');p.write_text(t)
p=r/'WebInstaller.html';s=p.read_text()
s=s.replace('V1.4.30 SID + BOOT KEY FIX (EXPERIMENTAL)','V1.4.31 SID WAH FIX (EXPERIMENTAL)')
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
assert n==1,'missing installer SHA'
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
s,n=re.subn(pat,lambda m:m[1]+'\n'+base64.b64encode(blob).decode()+'\n'+m[3],s,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pat,s,re.S)[2].strip())==blob
p.write_text(s);(r/'web/WebInstaller.html').write_text(s)
p=r/'BEETLE-MANAGER.html';manager=p.read_text().replace('V1.4.30','V1.4.31')
for tag,name in [('source-emulator','simulator.html'),('source-projectmanager','BEETLE-Project-Manager.html'),('source-installer','WebInstaller.html')]:
 q=rf'(<script id="{tag}" type="application/octet-stream">)(.*?)(</script>)'
 b=(r/name).read_bytes();manager,n=re.subn(q,lambda m:m[1]+base64.b64encode(b).decode()+m[3],manager,count=1,flags=re.S)
 assert n==1
 assert base64.b64decode(re.search(q,manager,re.S)[2])==b
p.write_text(manager)
manifest=r/'BEETLE-V1.4.31-BUILD-MANIFEST.json';j=json.loads(manifest.read_text());j.update({'version':'BEETLE V1.4.31','sid_regression_fix':'SID 6581/DIRTY/BASS neutral B phase lock + V1.4.29 cutoff/damping; deliberate B detune still works','sid_8580_noise_code_untouched':True,'webinstaller_embeds_firmware_sha256':sha,'hardware_validated':False});manifest.write_text(json.dumps(j,indent=2)+'\n')
print('OK firmware/embedded installer/manager:',sha)
