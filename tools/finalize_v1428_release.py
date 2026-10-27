#!/usr/bin/env python3
"""Synchronize V1.4.28 release assets; preserve complete web functionality."""
from pathlib import Path
import base64, hashlib, re, json
r=Path(__file__).resolve().parents[1]
fw=r/'BEETLE-V1.4.28-SAMPLE-STREAM-EXPERIMENTAL.fwsc'
blob=fw.read_bytes(); sha=hashlib.sha256(blob).hexdigest()
for name in ('simulator.html','web/simulator.html','BEETLE-Project-Manager.html','web/BEETLE-Project-Manager.html'):
 p=r/name
 s=p.read_text()
 s=s.replace('V1.4.27','V1.4.28')
 p.write_text(s)
# Embedded firmware must match the exact validated native package.
installer=r/'WebInstaller.html'
s=installer.read_text().replace('V1.4.27 CRY / BITCRUSH / RADIO (EXPERIMENTAL)', 'V1.4.28 USER SAMPLE STREAMING (EXPERIMENTAL)').replace('V1.4.27','V1.4.28')
assert len(re.findall(r"HASH_ET: '[0-9a-f]{64}'",s))==1
s=re.sub(r"(HASH_ET: ')[0-9a-f]{64}(')", lambda m:m[1]+sha+m[2],s,count=1)
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
s,n=re.subn(pat,lambda m:m[1]+'\n'+base64.b64encode(blob).decode()+'\n'+m[3],s,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pat,s,re.S).group(2).strip())==blob
installer.write_text(s);(r/'web/WebInstaller.html').write_text(s)
# All-in-one: only replace the three embedded tools, not the UI/runtime.
p=r/'BEETLE-MANAGER.html'; manager=p.read_text().replace('V1.4.27','V1.4.28')
for tag,file in [('source-emulator','simulator.html'),('source-projectmanager','BEETLE-Project-Manager.html'),('source-installer','WebInstaller.html')]:
 pat=rf'(<script id="{tag}" type="application/octet-stream">)(.*?)(</script>)'
 data=(r/file).read_bytes()
 manager,n=re.subn(pat,lambda m:m[1]+base64.b64encode(data).decode()+m[3],manager,count=1,flags=re.S)
 assert n==1
 assert base64.b64decode(re.search(pat,manager,re.S).group(2))==data
p.write_text(manager)
manifest={
 'release':'V1.4.28', 'firmware':fw.name,'fwsc_sha256':sha,'file_bytes':len(blob),
 'source':'JieLi native build from current source',
 'webinstaller_embeds_identical_firmware':True,
 'manager_embeds_current_simulator_project_manager_installer':True,
 'hardware_validated':False,
 'project_schema':'TRK1 V25',
 'sample_format':'BD4S (unchanged)',
 'new_user_samples':0,
 'recovery_validated':False,
}
(r/'BEETLE-V1.4.28-BUILD-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('PASS V1.4.28 website packaging and embedded FWSC',sha)
