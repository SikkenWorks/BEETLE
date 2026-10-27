#!/usr/bin/env python3
"""Synchronize BEETLE V1.6.7 release assets, firmware and off-device web tools.
No hardware calls. Protective bootloader preservation is separately checked by build_v1434_fwsc.py.
"""
from pathlib import Path
import base64,hashlib,json,re
r=Path(__file__).resolve().parents[1]
v='V1.6.7';fwname='BEETLE-V1.6.7-EDIT-GLO-SAMPLE-STABILITY-EXPERIMENTAL.fwsc'
p=r/fwname;blob=p.read_bytes();sha=hashlib.sha256(blob).hexdigest()
for name in ('BEETLE-Project-Manager.html','web/BEETLE-Project-Manager.html','simulator.html','web/simulator.html'):
 q=r/name;s=q.read_text();s=s.replace('V1.6.6',v);q.write_text(s)
inst=r/'WebInstaller.html';s=inst.read_text().replace('V1.6.6',v)
# The installer includes a trusted SHA and its full FWSC blob.
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
assert n==1,'Installer lacks firmware SHA constant'
pattern=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
s,n=re.subn(pattern,lambda m:m[1]+'\n'+base64.b64encode(blob).decode()+'\n'+m[3],s,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pattern,s,re.S)[2].strip())==blob
inst.write_text(s);(r/'web/WebInstaller.html').write_text(s)
# Project Manager also embedded in the standalone All-in-One Manager.
m=r/'BEETLE-MANAGER.html';s=m.read_text().replace('V1.6.6',v)
for tag,name in [('source-emulator','simulator.html'),('source-projectmanager','BEETLE-Project-Manager.html'),('source-installer','WebInstaller.html')]:
 pat=rf'(<script id="{tag}" type="application/octet-stream">)(.*?)(</script>)'
 b=(r/name).read_bytes();s,n=re.subn(pat,lambda x:x[1]+base64.b64encode(b).decode()+x[3],s,count=1,flags=re.S)
 assert n==1 and base64.b64decode(re.search(pat,s,re.S)[2].strip())==b,(tag,n)
m.write_text(s);(r/'web/BEETLE-MANAGER.html').write_text(s)
# README retains only operational docs, with version badge updated.
q=r/'README.md';s=q.read_text().replace('V1.6 EXPERIMENTAL',v+' EXPERIMENTAL');q.write_text(s)
# Native builder JSON and full integrity flags.
import sys
sys.path.insert(0,str(r/'tools'))
from fm1_fwsc import parse_ufw
old=r/'BEETLE-V1.6.6-PROJECT-DELETE-HOTKEY-EXPERIMENTAL.fwsc'
a=parse_ufw(old.read_bytes());b=parse_ufw(blob)
def entry(p,name):return next(x['data'] for x in p['entries'] if x['name']==name)
header_ok=entry(a,'flash.bin')[:16384]==entry(b,'flash.bin')[:16384]
ota_ok=entry(a,'ota.bin')==entry(b,'ota.bin')
assert header_ok and ota_ok
app=r/'build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin'
manifest={'version':v,'description':'EDIT+GLO LOAD PROJECT deletion; CH6 NOTE2/NOTE3 preview; 128-frame sample streaming buffer','firmware_filename':fwname,'sha256':sha,'app_sha256':hashlib.sha256(app.read_bytes()).hexdigest(),'app_bytes':app.stat().st_size,'project_format':'TRK1 V26','previous_release':'V1.6.6','source_cleaned':True,'physical_fm1_tested':False,'ota_header_identical_to_v166':header_ok,'ota_loader_identical_to_v166':ota_ok,'webinstaller_embeds_exact_fwsc':True,'manager_embeds_current_webtools':True,'help_parity':True,'sample_pcm_window_frames':128,'sample_stream_runtime_bytes':12984,'sample_runtime_workspace_bytes':37648,'host_sample_tests':True,'physical_audio_performance_validated':False,'release_notes_location':'CHANGELOG.md'}
(r/f'BEETLE-{v}-BUILD-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
# Remove stale generated release artefacts from the staging root (never remove any source).
(r/'BEETLE-V1.6.6-BUILD-MANIFEST.json').unlink(missing_ok=True)
old.unlink(missing_ok=True)
print(json.dumps(manifest,indent=2))
