#!/usr/bin/env python3
"""BEETLE V1.6.10 complete off-device finalize + embedded firmware verification."""
from pathlib import Path
import base64,hashlib,json,re,sys
r=Path(__file__).resolve().parents[1]
version='V1.6.10'
fwname=f'BEETLE-{version}-PROJECT-DELETE-VALIDATOR-FIX-EXPERIMENTAL.fwsc'
blob=(r/fwname).read_bytes();sha=hashlib.sha256(blob).hexdigest()
previous=r/'BEETLE-V1.6.9-DELETE-SELECTOR-SAFETY-EXPERIMENTAL.fwsc'
assert previous.is_file()
# Root webtool versions + source copies
for name in ['simulator.html','BEETLE-Project-Manager.html']:
 p=r/name;s=p.read_text().replace('V1.6.9',version);p.write_text(s)
 (r/'web'/name).write_text(s)
inst=r/'WebInstaller.html';s=inst.read_text().replace('V1.6.9',version)
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
assert n==1,'installer SHA marker not found'
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
s,n=re.subn(pat,lambda m:m[1]+'\n'+base64.b64encode(blob).decode()+'\n'+m[3],s,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pat,s,re.S)[2].strip())==blob
inst.write_text(s);(r/'web/WebInstaller.html').write_text(s)
m=r/'BEETLE-MANAGER.html';s=m.read_text().replace('V1.6.9',version)
for tag,name in [('source-emulator','simulator.html'),('source-projectmanager','BEETLE-Project-Manager.html'),('source-installer','WebInstaller.html')]:
 pat=rf'(<script id="{tag}" type="application/octet-stream">)(.*?)(</script>)'
 data=(r/name).read_bytes()
 s,n=re.subn(pat,lambda x:x[1]+base64.b64encode(data).decode()+x[3],s,count=1,flags=re.S)
 assert n==1 and base64.b64decode(re.search(pat,s,re.S)[2].strip())==data,tag
m.write_text(s);(r/'web/BEETLE-MANAGER.html').write_text(s)
sys.path.insert(0,str(r/'tools'))
from fm1_fwsc import parse_ufw,verify_fwsc
old=parse_ufw(previous.read_bytes());new,err=verify_fwsc(r/fwname);assert not err,err
newraw=parse_ufw(blob)
entry=lambda p,name:next(x['data'] for x in p['entries'] if x['name']==name)
assert entry(old,'flash.bin')[:16384]==entry(newraw,'flash.bin')[:16384]
assert entry(old,'ota.bin')==entry(newraw,'ota.bin')
app=r/'build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin'
appbytes=app.read_bytes();assert b'BEETLE V1.6.10' in appbytes
manifest={'version':version,
 'description':'FIX native page-39/40 validator; latched EDIT+SEL physical input; visible runtime version + input diagnostics',
 'firmware_filename':fwname,'sha256':sha,
 'app_sha256':hashlib.sha256(appbytes).hexdigest(),'app_bytes':len(appbytes),
 'app_headroom_bytes':581564-len(appbytes),'previous_release':'V1.6.9',
 'root_cause':'native page validator excluded DELETE modal page 39 and erase progress page 40',
 'fixed_native_page_39_40':True,'hardware_chord_latched_in_matrix_scanner':True,
 'startup_version_visible_on_fm1':True,'load_screen_key_diagnostic':True,
 'project_format':'TRK1 V26','global_user_samples_unchanged':True,
 'ota_header_identical_to_previous':True,'ota_loader_identical_to_previous':True,
 'native_build_success':True,'firmware_package_verified':True,
 'webinstaller_embeds_exact_fwsc':True,'manager_embeds_current_webtools':True,
 'physical_fm1_tested':False,'ota_recovery_proven':False,
 'release_notes_location':'CHANGELOG.md'}
(r/f'BEETLE-{version}-BUILD-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
(r/'FIRMWARE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
for f in r.glob('BEETLE-V1.*-BUILD-MANIFEST.json'):
 if f.name!=f'BEETLE-{version}-BUILD-MANIFEST.json':f.unlink()
for f in r.glob('BEETLE-V1.*-*.fwsc'):
 if f.name!=fwname:f.unlink()
print(json.dumps(manifest,indent=2))
