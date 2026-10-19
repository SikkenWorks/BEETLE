from pathlib import Path
import sys,hashlib,json
# Experimental compatibility-packer, NOT independent R0 hardware recovery proof.
source=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(source/'tools'))
import fm1_fwsc as f
old=source/'reference/BEETLE-RC1.0.5-PROTECTED-BOOT-REFERENCE.fwsc.reference'
app=source/'build/track-fm1-native-beetle-storage-lab-updater/app.bin'
out=source/'BEETLE-1.01-RC1.0.8-EXPERIMENTAL.fwsc'
appraw=app.read_bytes(); oldraw=old.read_bytes()
assert f.validate_app_binary(appraw)==[],f.validate_app_binary(appraw)
assets=f.extract_stock_assets(old)
flash=bytearray(f.build_flash_image_from_assets(appraw,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload']))
_,oldlogical=f.decode_fwsc(oldraw)
oldflash=f.parse_ufw(oldraw)['entries'][0]['data']
assert len(oldflash)==0x93000
# The installed firmware compares the full first 16KiB during OTA CHECK.
# Preserve the proven reference bytes *bit for bit*, including the uboot
# directory CRC words which the generic repacker normalizes differently.
changed=[i for i in range(0x4000) if flash[i]!=oldflash[i]]
assert changed==[0x20,0x21,0x22,0x23],changed[:40]
flash[:0x4000]=oldflash[:0x4000]
fw=f.build_fwsc(bytes(flash),assets['ota'],'FM-1_2108')
out.write_bytes(fw)
report,errors=f.verify_fwsc(out)
assert not errors,errors
pkg=f.parse_ufw(out.read_bytes())
assert pkg['product']=='FM-1_2108'
assert pkg['entries'][0]['data'][:0x4000]==oldflash[:0x4000]
assert pkg['entries'][0]['data'][0x4000:]==flash[0x4000:]
assert pkg['entries'][1]['data']==assets['ota']
dec=bytearray(pkg['entries'][0]['data'][0x4000:0x4000+0x120+len(appraw)])
f.sfc_xor(dec,0,len(dec),0,assets['key'])
assert dec[0x120:0x120+len(appraw)]==appraw
p_sha=hashlib.sha256(out.read_bytes()).hexdigest()
meta={'firmware':'BEETLE 1.01 RC1.0.8','identity':'FM-1_2108','target':'M-VAVE FM1 / JieLi AC791N','bytes':out.stat().st_size,'sha256':p_sha,'app_sha256':hashlib.sha256(appraw).hexdigest(),'reference_package':old.name,'reference_sha256':hashlib.sha256(oldraw).hexdigest(),'protected_header_sha256':hashlib.sha256(oldflash[:0x4000]).hexdigest(),'ota_sha256':hashlib.sha256(assets['ota']).hexdigest(),'container_shape_compliant':True,'preserved_reference_16KiB':True,'hardware_tested':False,'independent_recovery_proven':False}
(out.parent/'FIRMWARE-MANIFEST.json').write_text(json.dumps(meta,indent=2)+'\n')
print(json.dumps(meta,indent=2))
