#!/usr/bin/env python3
"""Fail-closed V1.6.11 release embedding / binary provenance; no FM1 I/O."""
from pathlib import Path
import base64,hashlib,json,re,sys
r=Path(__file__).resolve().parents[1]
version='V1.6.11'
fwname=f'BEETLE-{version}-AUDIO-SAFETY-EXPERIMENTAL.fwsc'
blob=(r/fwname).read_bytes();sha=hashlib.sha256(blob).hexdigest()
previous=r/'BEETLE-V1.6.10-PROJECT-DELETE-VALIDATOR-FIX-EXPERIMENTAL.fwsc'
assert previous.is_file()
for name in ['simulator.html','BEETLE-Project-Manager.html']:
 p=r/name;s=p.read_text().replace('V1.6.10',version);p.write_text(s)
 (r/'web'/name).write_text(s)
inst=r/'WebInstaller.html';s=inst.read_text().replace('V1.6.10',version)
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
assert n==1, 'installer SHA marker missing'
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
s,n=re.subn(pat,lambda m:m[1]+'\n'+base64.b64encode(blob).decode()+'\n'+m[3],s,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pat,s,re.S)[2].strip())==blob
inst.write_text(s);(r/'web/WebInstaller.html').write_text(s)
m=r/'BEETLE-MANAGER.html';s=m.read_text().replace('V1.6.10',version)
for tag,name in [('source-emulator','simulator.html'),('source-projectmanager','BEETLE-Project-Manager.html'),('source-installer','WebInstaller.html')]:
 pat=rf'(<script id="{tag}" type="application/octet-stream">)(.*?)(</script>)'
 data=(r/name).read_bytes();s,n=re.subn(pat,lambda x:x[1]+base64.b64encode(data).decode()+x[3],s,count=1,flags=re.S)
 assert n==1 and base64.b64decode(re.search(pat,s,re.S)[2].strip())==data,tag
m.write_text(s);(r/'web/BEETLE-MANAGER.html').write_text(s)
sys.path.insert(0,str(r/'tools'))
from fm1_fwsc import parse_ufw,verify_fwsc
old=parse_ufw(previous.read_bytes());new,err=verify_fwsc(r/fwname);assert not err,err
newraw=parse_ufw(blob);entry=lambda p,name:next(x['data'] for x in p['entries'] if x['name']==name)
assert entry(old,'flash.bin')[:16384]==entry(newraw,'flash.bin')[:16384]
assert entry(old,'ota.bin')==entry(newraw,'ota.bin')
app=r/'build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin';appbytes=app.read_bytes();assert b'BEETLE V1.6.11' in appbytes
manifest={'version':version,'classification':'CRITICAL AUDIO SAFETY - EXPERIMENTAL - hardware not tested',
 'firmware_filename':fwname,'sha256':sha,
 'app_sha256':hashlib.sha256(appbytes).hexdigest(),'app_bytes':len(appbytes),
 'app_headroom_bytes':581564-len(appbytes),'previous_release':'V1.6.10',
 'source_confirmed_faults':['stale foreground ADC/master command while ISR continues audio',
 'unbounded copy interrupt lock during live song',
 'ineffective ETCH voice shedding on BEETLE DMA deadline',
 'automatic DMA audio fault restart', 'cross-lane NOTE3 clipboard overwrite'],
 'physical_root_cause_proven':False,'physical_tested':False,'acoustic_peak_measured':False,
 'audio_safety_strategy':['output zero when last valid MASTER ADC >80ms old',
 'clamp invalid ADC to mute, bound positive gain slew',
 'hardware analog PC6 off at first late DMA half',
 'latch until HOME+PLAY manual rearm',
 'refuse copy during active transport'],
 'project_format':'TRK1 V26','global_user_samples_unchanged':True,
 'ota_header_identical_to_previous':True,'ota_loader_identical_to_previous':True,
 'native_build_success':True,'firmware_package_verified':True,
 'webinstaller_embeds_exact_fwsc':True,'manager_embeds_current_webtools':True,
 'ota_recovery_proven':False,'release_notes_location':'CHANGELOG.md',
 'manual':'README.md','safety_analysis':'docs/V1611-AUDIO-INCIDENT-FORENSICS.md'}
(r/f'BEETLE-{version}-BUILD-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
(r/'FIRMWARE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
for f in r.glob('BEETLE-V1.*-BUILD-MANIFEST.json'):
 if f.name!=f'BEETLE-{version}-BUILD-MANIFEST.json':f.unlink()
for f in r.glob('BEETLE-V1.*-*.fwsc'):
 if f.name!=fwname:f.unlink()
print(json.dumps(manifest,indent=2))
