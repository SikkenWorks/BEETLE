#!/usr/bin/env python3
"""Offline packaging of BEETLE V1.8 EXPERIMENTAL; preserves existing protected assets."""
from pathlib import Path
import sys,re,base64,hashlib,json
r=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(r/'tools'))
import fm1_fwsc as fw
ref=r/'reference/BEETLE-V1.7.14-WARM-DMA-NEW-PROJECT-EXPERIMENTAL.fwsc'
app=r/'build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin'
out=r/'BEETLE-V1.8-STABILITY-EXPERIMENTAL.fwsc'
old,errors=fw.verify_fwsc(ref)
assert not errors and old['product']=='FM-1_2123',errors
assert fw.validate_app_audit(app,Path(str(app)+'.audit.json'))[1]==[]
assets=fw.extract_stock_assets(ref)
oldpkg=fw.parse_ufw(ref.read_bytes());old_flash=next(e['data'] for e in oldpkg['entries'] if e['name']=='flash.bin')
new_flash=bytearray(fw.build_flash_image_from_assets(app.read_bytes(),assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload']))
new_flash[:0x4000]=old_flash[:0x4000]
blob=fw.build_fwsc(bytes(new_flash),assets['ota'],'FM-1_2123')
out.write_bytes(blob)
parsed,errors=fw.verify_fwsc(out)
assert not errors and parsed['product']=='FM-1_2123',errors
entries=fw.parse_ufw(blob)['entries']
flash=next(e['data'] for e in entries if e['name']=='flash.bin')
ota=next(e['data'] for e in entries if e['name']=='ota.bin')
plain=bytearray(flash[fw.APP_AREA_FLASH:]);fw.sfc_xor(plain,0,len(plain),0,assets['key'])
assert plain[0x120:0x120+app.stat().st_size]==app.read_bytes()
assert flash[:0x4000]==old_flash[:0x4000] and ota==assets['ota']
assert fw.compare_packages(ref,out)['same_container_shape']
sha=lambda x:hashlib.sha256(x).hexdigest()
installer=r/'WebInstaller.html';h=installer.read_text().replace('V1.7.14','V1.8').replace('1.7.14','1.8')
h,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'",f"HASH_ET: '{sha(blob)}'",h,count=1);assert n==1
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
h,n=re.subn(pat,lambda m:m.group(1)+base64.b64encode(blob).decode()+m.group(3),h,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pat,h,re.S).group(2).strip())==blob
installer.write_text(h)
for dst in (r/'web/WebInstaller.html',r/'BEETLE-V1.8-WebInstaller.html'):dst.write_text(h)
for src,dst in [('simulator.html','BEETLE-V1.8-WebSimulator.html'),('BEETLE-Project-Manager.html','BEETLE-V1.8-Project-Manager.html')]:
 (r/dst).write_bytes((r/src).read_bytes())
manager=r/'BEETLE-MANAGER.html';h=manager.read_text()
for tag,name in [('source-emulator','simulator.html'),('source-projectmanager','BEETLE-Project-Manager.html'),('source-installer','WebInstaller.html')]:
 pattern=rf'(<script id="{tag}" type="application/octet-stream">)(.*?)(</script>)'
 data=(r/name).read_bytes();h,n=re.subn(pattern,lambda m:m.group(1)+base64.b64encode(data).decode()+m.group(3),h,count=1,flags=re.S)
 assert n==1 and base64.b64decode(re.search(pattern,h,re.S).group(2))==data
h=re.sub(r'V1\.7\.(?:12|13|14)', 'V1.8',h)
manager.write_text(h)
for dst in (r/'web/BEETLE-MANAGER.html',r/'BEETLE-V1.8-MANAGER.html'):dst.write_text(h)
headroom=581564-app.stat().st_size
m={
 'version':'V1.8','identity':'FM-1_2123','previous_release':'V1.7.14',
 'firmware_filename':out.name,'firmware_sha256':sha(blob),'app_sha256':sha(app.read_bytes()),
 'app_bytes':app.stat().st_size,'app_headroom_bytes':headroom,'project_schema':'TRK1 V26',
 'protected_16k_header_identical':True,'ota_loader_identical':True,
 'firmware_container_shape_identical':True,'native_full_build_compiled':True,
 'native_readonly_build_compiled':True,'blue_rc12_battery_policy_port':True,
 'blue_rc12_pat_position_and_help_card_layout':True,
 'fused_sample_window_pcm_isr':True,'foreground_serial_pending_snapshot':True,
 'pinned_chunk_verification_reuse':True,'deferred_decoder_stop':True,
 'warm_new_dma_from_v1714_retained':True,'hardware_tested':False,
 'dma_deadlines_eliminated_on_hardware':False,'peak_safety_physically_verified':False,
 'readme':'README.md','changelog':'CHANGELOG.md','validation':'V1.8-VALIDATION.md'}
for p in (r/'FIRMWARE-MANIFEST.json',r/'BEETLE-V1.8-BUILD-MANIFEST.json'):p.write_text(json.dumps(m,indent=2)+'\n')
print(json.dumps(m,indent=2))
