#!/usr/bin/env python3
"""Package a pre-reSID V1.4.1 Project-Resident One-Shots. OFFLINE ONLY.

Uses the verified ORIGINAL 1.3.8 FWSC's protected bootloader/header
and unchanged OTA loader. Keeps the original FM-1_2123 product identity.
This is NOT a hardware flash/recovery guarantee.
"""
from pathlib import Path
import sys,hashlib,json
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw
if len(sys.argv)!=4:
    raise SystemExit('usage: build_v140_fwsc.py ORIGINAL-V1.3.8.fwsc APP.bin OUT.fwsc')
ref_path,app_path,out = (Path(x) for x in sys.argv[1:])
ref,err=fw.verify_fwsc(ref_path)
if err or ref['product'] != 'FM-1_2123':
    raise SystemExit('Original V1.3.8 package failed validation: '+str(err))
app=app_path.read_bytes()
audit,err=fw.validate_app_audit(app_path,Path(str(app_path)+'.audit.json'))
if err or not audit:raise SystemExit('App ELF audit failed: '+str(err))
assets=fw.extract_stock_assets(ref_path)
old=fw.parse_ufw(ref_path.read_bytes())
old_flash=next(e['data'] for e in old['entries'] if e['name']=='flash.bin')
new_flash=bytearray(fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload']))
new_flash[:0x4000]=old_flash[:0x4000]
output=fw.build_fwsc(bytes(new_flash),assets['ota'],'FM-1_2123')
out.write_bytes(output)
r,err=fw.verify_fwsc(out)
if err or r['product']!='FM-1_2123':
    out.unlink();raise SystemExit('Result failed validation: '+str(err))
new=fw.parse_ufw(output)
flash=next(e['data'] for e in new['entries'] if e['name']=='flash.bin')
ota=next(e['data'] for e in new['entries'] if e['name']=='ota.bin')
dec=bytearray(flash[fw.APP_AREA_FLASH:]);fw.sfc_xor(dec,0,len(dec),0,assets['key'])
checks={'protected_header_identical':flash[:0x4000]==old_flash[:0x4000],
        'original_ota_loader_identical':ota==assets['ota'],
        'decrypted_app_matches_build':dec[0x120:0x120+len(app)]==app,
        'container_shape_unchanged':fw.compare_packages(ref_path,out)['same_container_shape'],
        'fwsc_verified':not err, 'app_audit':bool(audit)}
if not all(checks.values()):
    out.unlink();raise SystemExit('Package changed critical assets: '+str(checks))
sha=lambda x:hashlib.sha256(x).hexdigest()
manifest={'version':'BEETLE V1.4.1','baseline':'original BEETLE V1.3.8 native pre-reSID','identity':'FM-1_2123','bytes':len(output),
'sha256':sha(output),'reference_sha256':sha(ref_path.read_bytes()),'app_sha256':sha(app),'app_bytes':len(app),
'reSID_native':False,'ymfm_native':False,'sustain_100_ui':'1.00',
'checks':checks,'uac_6stem_shared_clock':True,'uac_64bit_interpolation_removed':True,'sample_slots':13,'sample_payload_capacity_bytes':6144,'sample_project_schema_with_samples':23,'sample_audio_format':'22050Hz mono 8-bit companded ROM-compatible','legacy_project_schema':22,'real_hardware_tested':False,'ota_recovery_proven':False}
(out.parent/'BEETLE-V1.4.1-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest,indent=2))
