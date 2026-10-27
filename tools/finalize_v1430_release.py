#!/usr/bin/env python3
"""Synchronize V1.4.30 user-facing assets and embed audited firmware. No FM1 I/O."""
from pathlib import Path
import re, base64, hashlib, json
r=Path(__file__).resolve().parents[1]
fw=r/'BEETLE-V1.4.30-SID-BOOT-EXPERIMENTAL.fwsc'
blob=fw.read_bytes();digest=hashlib.sha256(blob).hexdigest()
for name in ('BEETLE-Project-Manager.html','web/BEETLE-Project-Manager.html','simulator.html','web/simulator.html'):
 p=r/name;html=p.read_text().replace('V1.4.28','V1.4.30');p.write_text(html)
p=r/'WebInstaller.html'
s=p.read_text().replace('V1.4.28 USER SAMPLE STREAMING (EXPERIMENTAL)','V1.4.30 SID + BOOT KEY FIX (EXPERIMENTAL)').replace('V1.4.28','V1.4.30')
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+digest+m[2],s,count=1)
assert n==1
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
s,n=re.subn(pat,lambda m:m[1]+'\n'+base64.b64encode(blob).decode()+'\n'+m[3],s,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pat,s,re.S)[2].strip())==blob
p.write_text(s);(r/'web/WebInstaller.html').write_text(s)
p=r/'BEETLE-MANAGER.html';manager=p.read_text().replace('V1.4.28','V1.4.30')
for tag,name in [('source-emulator','simulator.html'),('source-projectmanager','BEETLE-Project-Manager.html'),('source-installer','WebInstaller.html')]:
 pat=rf'(<script id="{tag}" type="application/octet-stream">)(.*?)(</script>)'
 b=(r/name).read_bytes()
 manager,n=re.subn(pat,lambda m:m[1]+base64.b64encode(b).decode()+m[3],manager,count=1,flags=re.S)
 assert n==1
 assert base64.b64decode(re.search(pat,manager,re.S)[2])==b
p.write_text(manager)
manifest=r/'BEETLE-V1.4.30-BUILD-MANIFEST.json'
doc=json.loads(manifest.read_text());doc.update({'version':'V1.4.30','sid_changes':['independent oscillator B phase', 'optional hard sync, triangle ring and combined wave', 'ROM 6581/8580 filter curves', '3 filter products changed to 32-bit'], 'boot_project_key_guard':'FM1 queued note edges drained and held keys suppressed until physical release','webinstaller_embeds_firmware_sha256':digest,'hardware_validated':False})
manifest.write_text(json.dumps(doc,indent=2)+'\n')
assert hashlib.sha256(blob).hexdigest()==digest
print('PASS V1.4.30 embedded FWSC and three all-in-one tools:',digest)
