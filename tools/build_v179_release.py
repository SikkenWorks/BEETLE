#!/usr/bin/env python3
"""Build V1.7.9 from verified V1.7.7 experimental package; preserve loader."""
from pathlib import Path
import sys,re,base64,hashlib,json
r=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(r/'tools'))
import fm1_fwsc as fw
ref=r/'reference/BEETLE-V1.7.7-FWSC-BUILD-REFERENCE.fwsc.reference'
app=r/'build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin'
out=r/'BEETLE-V1.7.9-FAST-NEW-PROJECT-EXPERIMENTAL.fwsc'
old,errors=fw.verify_fwsc(ref)
assert not errors and old['product']=='FM-1_2123',errors
assert fw.validate_app_audit(app,Path(str(app)+'.audit.json'))[1]==[]
assets=fw.extract_stock_assets(ref)
old_flash=next(e['data'] for e in fw.parse_ufw(ref.read_bytes())['entries'] if e['name']=='flash.bin')
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
installer=r/'WebInstaller.html';h=installer.read_text().replace('V1.7.8','V1.7.9')
h,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'",f"HASH_ET: '{sha(blob)}'",h,count=1);assert n==1
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
h,n=re.subn(pat,lambda m:m.group(1)+'\n'+base64.b64encode(blob).decode()+'\n'+m.group(3),h,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pat,h,re.S).group(2).strip())==blob
installer.write_text(h)
wp=r/'web/WebInstaller.html'
if wp.exists():wp.write_text(h)
manager=r/'BEETLE-MANAGER.html';h=manager.read_text().replace('V1.7.8','V1.7.9')
for tag,name in [('source-emulator','simulator.html'),('source-projectmanager','BEETLE-Project-Manager.html'),('source-installer','WebInstaller.html')]:
 pattern=rf'(<script id="{tag}" type="application/octet-stream">)(.*?)(</script>)'
 data=(r/name).read_bytes();h,n=re.subn(pattern,lambda m:m.group(1)+base64.b64encode(data).decode()+m.group(3),h,count=1,flags=re.S)
 assert n==1 and base64.b64decode(re.search(pattern,h,re.S).group(2))==data
manager.write_text(h)
wp=r/'web/BEETLE-MANAGER.html'
if wp.exists():wp.write_text(h)
m={
 'version':'V1.7.9','identity':'FM-1_2123','status':'Experimental software build; physical audio safety unverified',
 'strategy':'Eliminate duplicate full LCD repaint; single-probe, audited V2 index/free-slot scan; add on-device phase timing; retain DMA safe restart and V1.6.10 oscillator sound',
 'app_bytes':app.stat().st_size,'headroom_bytes':581564-app.stat().st_size,
 'app_sha256':sha(app.read_bytes()),'fwsc_sha256':sha(blob),'fwsc_filename':out.name,
 'protected_flash_header_identical':True,'ota_loader_identical':True,'container_shape_identical':True,
 'project_format':'TRK1 V26','v25_decode_supported':True,'sample_storage_format_changed':False,
 'new_project_dma_suspended_before_flash_and_reset':True,
 'new_project_single_tracker_paint':True,
 'new_project_storage_v2_single_audited_slot_scan':True,
 'input_quarantine_ms':320,'master_adc_checked_between_lcd_halves':True,
 'legacy_screen_compositor_retained':True,'legacy_step_sequencer_retained':True,
 'all_twelve_osc_gain_q8':[105,245,256,345,410,403,631,347,525,214,181,244],
 'project_source_included':False,'all_12_type_48_scenes_bit_identical_to_v1610':True,
 'legacy_full_12_type_balance_test_pass':False,'on_device_new_project_phase_metrics':True,'physical_new_project_latency_measured':False,
 'physical_stability_verified':False,'physical_peak_safety_verified':False,'independent_ota_recovery_verified':False
}
(r/'FIRMWARE-MANIFEST.json').write_text(json.dumps(m,indent=2)+'\n')
(r/'BEETLE-V1.7.9-BUILD-MANIFEST.json').write_text(json.dumps(m,indent=2)+'\n')
print(json.dumps(m,indent=2))
