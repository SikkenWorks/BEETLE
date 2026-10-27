from pathlib import Path
import base64,hashlib,re,json,sys
r=Path('/mnt/data/beetle_v168_work');v='V1.6.8'
fwname='BEETLE-V1.6.8-PROJECT-SELECTOR-HOTKEY-FIX-EXPERIMENTAL.fwsc'
blob=(r/fwname).read_bytes();fwsha=hashlib.sha256(blob).hexdigest()
for name in ('simulator.html','BEETLE-Project-Manager.html'):
 p=r/name;s=p.read_text();s=s.replace('V1.6.7',v).replace('V1.6.6 PROJECT DELETE',v+' PROJECT DELETE');p.write_text(s)
 (r/'web'/name).write_text(s)
inst=r/'WebInstaller.html';s=inst.read_text().replace('V1.6.7',v)
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+fwsha+m[2],s,count=1)
assert n==1
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
s,n=re.subn(pat,lambda m:m[1]+'\n'+base64.b64encode(blob).decode()+'\n'+m[3],s,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pat,s,re.S)[2].strip())==blob
inst.write_text(s);(r/'web/WebInstaller.html').write_text(s)
manager=r/'BEETLE-MANAGER.html';s=manager.read_text().replace('V1.6.7',v)
for tag,name in [('source-emulator','simulator.html'),('source-projectmanager','BEETLE-Project-Manager.html'),('source-installer','WebInstaller.html')]:
 pat=rf'(<script id="{tag}" type="application/octet-stream">)(.*?)(</script>)'
 b=(r/name).read_bytes();s,n=re.subn(pat,lambda m:m[1]+base64.b64encode(b).decode()+m[3],s,count=1,flags=re.S)
 assert n==1 and base64.b64decode(re.search(pat,s,re.S)[2].strip())==b,tag
manager.write_text(s);(r/'web/BEETLE-MANAGER.html').write_text(s)
from pathlib import Path
sys.path.insert(0,str(r/'tools'))
from fm1_fwsc import parse_ufw
ref=parse_ufw((r/'reference/BEETLE-RC1.0.5-PROTECTED-BOOT-REFERENCE.fwsc.reference').read_bytes());new=parse_ufw(blob)
get=lambda pkg,name:next(x['data'] for x in pkg['entries'] if x['name']==name)
assert get(ref,'flash.bin')[:0x4000]==get(new,'flash.bin')[:0x4000]
assert get(ref,'ota.bin')==get(new,'ota.bin')
app=r/'build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin'
manifest={'version':v,'description':'Boot project selector defers SEL to release and blocks chords; EDIT+SEL delete in LOAD, no inadvertent NEW project','firmware_filename':fwname,'sha256':fwsha,'app_sha256':hashlib.sha256(app.read_bytes()).hexdigest(),'app_bytes':app.stat().st_size,'app_headroom_bytes':581564-app.stat().st_size,'project_format':'TRK1 V26','previous_release':'V1.6.7','source_cleaned':True,'physical_fm1_tested':False,'ota_header_identical_to_v167':True,'ota_loader_identical_to_v167':True,'webinstaller_embeds_exact_fwsc':True,'manager_embeds_current_webtools':True,'help_parity':True,'startup_project_chord_regression':True,'physical_project_delete_tested':False,'release_notes_location':'CHANGELOG.md'}
(r/f'BEETLE-{v}-BUILD-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
for path in r.glob('BEETLE-V1.6.[67]-BUILD-MANIFEST.json'):path.unlink()
for path in r.glob('BEETLE-V1.6.[67]-*.fwsc'):path.unlink()
print(json.dumps(manifest,indent=2))
