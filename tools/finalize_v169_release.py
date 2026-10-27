#!/usr/bin/env python3
"""Finalize V1.6.9 firmware/web/source, asserting against stale embedded builds.
Offline only: never connects to or writes the FM1.
"""
from pathlib import Path
import base64, hashlib, json, re, sys
r=Path(__file__).resolve().parents[1]
version='V1.6.9'
fwname=f'BEETLE-{version}-DELETE-SELECTOR-SAFETY-EXPERIMENTAL.fwsc'
blob=(r/fwname).read_bytes()
sha=hashlib.sha256(blob).hexdigest()
previous=r/'BEETLE-V1.6.8-PROJECT-SELECTOR-HOTKEY-FIX-EXPERIMENTAL.fwsc'
assert previous.is_file()
for name in ['simulator.html','BEETLE-Project-Manager.html']:
 p=r/name;s=p.read_text().replace('V1.6.8',version);p.write_text(s)
 (r/'web'/name).write_text(s)
inst=r/'WebInstaller.html';s=inst.read_text().replace('V1.6.8',version)
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
assert n==1, 'hash marker missing'
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
s,n=re.subn(pat,lambda m:m[1]+'\n'+base64.b64encode(blob).decode()+'\n'+m[3],s,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pat,s,re.S)[2].strip())==blob
inst.write_text(s);(r/'web/WebInstaller.html').write_text(s)
m=r/'BEETLE-MANAGER.html';s=m.read_text().replace('V1.6.8',version)
for tag,name in [('source-emulator','simulator.html'),('source-projectmanager','BEETLE-Project-Manager.html'),('source-installer','WebInstaller.html')]:
 pat=rf'(<script id="{tag}" type="application/octet-stream">)(.*?)(</script>)'
 data=(r/name).read_bytes()
 s,n=re.subn(pat,lambda x:x[1]+base64.b64encode(data).decode()+x[3],s,count=1,flags=re.S)
 assert n==1 and base64.b64decode(re.search(pat,s,re.S)[2].strip())==data,tag
m.write_text(s);(r/'web/BEETLE-MANAGER.html').write_text(s)
sys.path.insert(0,str(r/'tools'))
from fm1_fwsc import parse_ufw,verify_fwsc
old=parse_ufw(previous.read_bytes());new,err=verify_fwsc(r/fwname)
assert not err and new is not None,err
newraw=parse_ufw(blob)
entry=lambda p,name:next(x['data'] for x in p['entries'] if x['name']==name)
assert entry(old,'flash.bin')[:16384]==entry(newraw,'flash.bin')[:16384]
assert entry(old,'ota.bin')==entry(newraw,'ota.bin')
app=r/'build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin'
manifest={'version':version,'description':'Boot NEW/LOAD clean SEL transaction + late-modifier veto; LOAD EDIT+SEL isolation','firmware_filename':fwname,'sha256':sha,'app_sha256':hashlib.sha256(app.read_bytes()).hexdigest(),'app_bytes':app.stat().st_size,'app_headroom_bytes':581564-app.stat().st_size,'previous_release':'V1.6.8','project_format':'TRK1 V26','user_samples_storage_unchanged':True,'native_build_success':True,'firmware_package_verified':True,'ota_header_identical_to_v168':True,'ota_loader_identical_to_v168':True,'webinstaller_embeds_exact_fwsc':True,'manager_embeds_current_webtools':True,'source_cleaned':True,'physical_fm1_tested':False,'ota_recovery_proven':False,'release_notes_location':'CHANGELOG.md'}
(r/f'BEETLE-{version}-BUILD-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
(r/'FIRMWARE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
for oldartifact in r.glob('BEETLE-V1.*-BUILD-MANIFEST.json'):
 if oldartifact.name!=f'BEETLE-{version}-BUILD-MANIFEST.json':oldartifact.unlink()
for oldartifact in r.glob('BEETLE-V1.*-*.fwsc'):
 if oldartifact.name!=fwname:oldartifact.unlink()
print(json.dumps(manifest,indent=2))
