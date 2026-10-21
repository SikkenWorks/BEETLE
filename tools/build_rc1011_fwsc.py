#!/usr/bin/env python3
"""Pack the audited RC1.0.11 app using RC1.0.10 boot+OTA, no device access."""
import hashlib,json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw
if len(sys.argv)!=4:raise SystemExit('usage: build_rc1011_fwsc.py REFERENCE_RC1010.fwsc APP.bin OUTPUT_RC1011.fwsc')
reference,app_path,out=map(Path,sys.argv[1:]);ID='FM-1_2111'
ref,errors=fw.verify_fwsc(reference)
if errors or ref['product']!='FM-1_2110':raise SystemExit('Bad RC1.0.10 reference '+str(errors))
app=app_path.read_bytes();audit,errors=fw.validate_app_audit(app_path,Path(str(app_path)+'.audit.json'))
if errors or not audit:raise SystemExit('Bad APP audit '+str(errors))
assets=fw.extract_stock_assets(reference)
old=fw.parse_ufw(reference.read_bytes());oldflash=next(e['data'] for e in old['entries'] if e['name']=='flash.bin')
newflash=bytearray(fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload']))
newflash[:0x4000]=oldflash[:0x4000]
data=fw.build_fwsc(bytes(newflash),assets['ota'],ID)
out.write_bytes(data)
r,errors=fw.verify_fwsc(out)
if errors or r['product']!=ID:out.unlink();raise SystemExit('Failed structural verification '+str(errors))
cmp=fw.compare_packages(reference,out)
new=fw.parse_ufw(data);flash=next(e['data'] for e in new['entries'] if e['name']=='flash.bin');ota=next(e['data'] for e in new['entries'] if e['name']=='ota.bin')
dec=bytearray(flash[fw.APP_AREA_FLASH:]);fw.sfc_xor(dec,0,len(dec),0,assets['key'])
checks={'header_same':flash[:0x4000]==oldflash[:0x4000],'loader_same':ota==assets['ota'],'app_same':dec[0x120:0x120+len(app)]==app,'container_same':cmp['same_container_shape'],'verified':not errors,'app_audit':audit}
if not all(checks.values()):out.unlink();raise SystemExit('FAILED: '+str(checks))
manifest={'firmware':'BEETLE 1.01 RC1.0.11 SONG CHAINS','identity':ID,'schema':19,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'app_sha256':hashlib.sha256(app).hexdigest(),'protected_header_sha256':hashlib.sha256(flash[:0x4000]).hexdigest(),'ota_sha256':hashlib.sha256(ota).hexdigest(),'source_reference':reference.name,'source_reference_sha256':hashlib.sha256(reference.read_bytes()).hexdigest(),'checks':checks,'hardware_tested':False,'recovery_proven':False,'type':'EXPERIMENTAL'}
(out.parent/'FIRMWARE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps(manifest,indent=2))
