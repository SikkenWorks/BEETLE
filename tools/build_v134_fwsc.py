#!/usr/bin/env python3
"""Pack V1.3.4 from the verified V1.3 container, preserving OTA and first 16KiB.
This is a firmware file transformation, NEVER a hardware flash or certification.
"""
import hashlib,json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw
if len(sys.argv)!=4:raise SystemExit('usage: build_v13_fwsc.py V1.3.3.fwsc app.bin V1.3.4.fwsc')
reference,app_path,out=map(Path,sys.argv[1:]); ID='FM-1_2119'
ref,err=fw.verify_fwsc(reference)
if err or ref['product']!='FM-1_2118':raise SystemExit(f'Untrusted V1.3.3 reference: {err}')
app=app_path.read_bytes();audit,err=fw.validate_app_audit(app_path,Path(str(app_path)+'.audit.json'))
if err or not audit:raise SystemExit('APP audit failed '+str(err))
assets=fw.extract_stock_assets(reference)
old=fw.parse_ufw(reference.read_bytes());oldflash=next(e['data'] for e in old['entries'] if e['name']=='flash.bin')
newflash=bytearray(fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload']))
newflash[:0x4000]=oldflash[:0x4000]
data=fw.build_fwsc(bytes(newflash),assets['ota'],ID);out.write_bytes(data)
r,err=fw.verify_fwsc(out)
if err or r['product']!=ID:out.unlink();raise SystemExit('Invalid firmware package '+str(err))
cmp=fw.compare_packages(reference,out)
new=fw.parse_ufw(data);flash=next(e['data'] for e in new['entries'] if e['name']=='flash.bin');ota=next(e['data'] for e in new['entries'] if e['name']=='ota.bin')
dec=bytearray(flash[fw.APP_AREA_FLASH:]);fw.sfc_xor(dec,0,len(dec),0,assets['key'])
checks={'header_same':flash[:0x4000]==oldflash[:0x4000],'loader_same':ota==assets['ota'],'app_same':dec[0x120:0x120+len(app)]==app,'container_same':cmp['same_container_shape'],'verified':not err,'app_audit':audit}
if not all(checks.values()):out.unlink();raise SystemExit('Verification failed '+str(checks))
sha=lambda buf:hashlib.sha256(buf).hexdigest()
manifest={'firmware':'BEETLE V1.3.4 THREE PER-STEP FX + PROJECT-WIDE SPARSE FX POOL (EXPERIMENTAL)','identity':ID,'schema':21,'bytes':len(data),'sha256':sha(data),'app_sha256':sha(app),'protected_header_sha256':sha(flash[:0x4000]),'ota_sha256':sha(ota),'source_reference':reference.name,'source_reference_sha256':sha(reference.read_bytes()),'extra_fx_slots':2,'total_fx_per_step':3,'extra_fx_pool_cap':48,'checks':checks,'wave_balance':'fixed per-type and per-VA-shape calibration, not dynamic AGC','usb_clock_sync':'fractional Q16 light interpolation, 44/45 fixed USB packet cadence','usb_audio':{'interface':'USB Audio Class 1 capture, USB endpoint 4 IN','sample_rate':44100,'channels':'2-channel master with USB MULTITRACK OFF / six independent mono channels when ON','bits':16,'hardware_verified':False},'hardware_tested':False,'recovery_proven':False,'type':'EXPERIMENTAL'}
(out.parent/'FIRMWARE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest,indent=2))
