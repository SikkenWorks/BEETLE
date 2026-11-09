#!/usr/bin/env python3
"""Package BEETLE V1.8.3 app+new open-source app-only Felucca-lineage loader.
The protected 16 KiB boothead remains identical to the known BEETLE baseline.
"""
from pathlib import Path
import sys,re,base64,hashlib,json
r=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(r/'tools'))
import fm1_fwsc as fw
sha=lambda b:hashlib.sha256(b).hexdigest()
ref=r/'reference/BEETLE-V1.7.14-WARM-DMA-NEW-PROJECT-EXPERIMENTAL.fwsc'
app=r/'build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin'
loader=r/'build/v183-loader/ota.bin'
out=r/'BEETLE-V1.8.3-DUAL-FLASH-EXPERIMENTAL.fwsc'
assert app.exists() and loader.exists(),'build app and loader before packaging'
old,errors=fw.verify_fwsc(ref)
assert not errors and old['product']=='FM-1_2123',errors
assert fw.validate_app_audit(app,Path(str(app)+'.audit.json'))[1]==[]
assets=fw.extract_stock_assets(ref)
oldpkg=fw.parse_ufw(ref.read_bytes())
old_flash=next(e['data'] for e in oldpkg['entries'] if e['name']=='flash.bin')
new_flash=bytearray(fw.build_flash_image_from_assets(app.read_bytes(),assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload']))
new_flash[:0x4000]=old_flash[:0x4000]
new_loader=loader.read_bytes()
wrap=fw.validate_ota(new_loader)
assert all(wrap.get(k) for k in ('outer_header_ok','inner_header_ok','outer_data_crc_ok','inner_data_crc_ok','inner_load_ok'))
assert wrap['encoding']=='lz4-blocks'
assert sha(new_loader)!=sha(assets['ota']),'loader did not change'
blob=fw.build_fwsc(bytes(new_flash),new_loader,'FM-1_2123')
out.write_bytes(blob)
parsed,errors=fw.verify_fwsc(out)
assert not errors and parsed['product']=='FM-1_2123',errors
entries=fw.parse_ufw(blob)['entries']
flash=next(e['data'] for e in entries if e['name']=='flash.bin')
ota=next(e['data'] for e in entries if e['name']=='ota.bin')
plain=bytearray(flash[fw.APP_AREA_FLASH:]);fw.sfc_xor(plain,0,len(plain),0,assets['key'])
assert plain[0x120:0x120+app.stat().st_size]==app.read_bytes()
assert flash[:0x4000]==old_flash[:0x4000] and ota==new_loader
# Refresh tools that embed the binary, preserve all existing workflows.
installer=r/'WebInstaller.html';h=re.sub(r'V1\.8\.2','V1.8.3',installer.read_text())
h,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'",f"HASH_ET: '{sha(blob)}'",h,count=1);assert n==1
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
h,n=re.subn(pat,lambda m:m.group(1)+base64.b64encode(blob).decode()+m.group(3),h,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pat,h,re.S).group(2).strip())==blob
installer.write_text(h)
(r/'BEETLE-V1.8.3-WebInstaller.html').write_text(h)
(r/'web/WebInstaller.html').write_text(h)
(r/'BEETLE-V1.8.3-WebSimulator.html').write_bytes((r/'simulator.html').read_bytes())
(r/'BEETLE-V1.8.3-Project-Manager.html').write_bytes((r/'BEETLE-Project-Manager.html').read_bytes())
manager=r/'BEETLE-MANAGER.html';h=manager.read_text()
h=re.sub(r'V1\.8\.[12]', 'V1.8.3',h)
for tag,name in [('source-emulator','simulator.html'),('source-projectmanager','BEETLE-Project-Manager.html'),('source-installer','WebInstaller.html')]:
 pattern=rf'(<script id="{tag}" type="application/octet-stream">)(.*?)(</script>)'
 data=(r/name).read_bytes();h,n=re.subn(pattern,lambda m:m.group(1)+base64.b64encode(data).decode()+m.group(3),h,count=1,flags=re.S)
 assert n==1 and base64.b64decode(re.search(pattern,h,re.S).group(2))==data
manager.write_text(h)
for dst in (r/'web/BEETLE-MANAGER.html',r/'BEETLE-V1.8.3-MANAGER.html'):dst.write_text(h)
# Historical .fwsc files are retained as comparators, never silently replaced.
m={'version':'V1.8.3','identity':'FM-1_2123','firmware_filename':out.name,
 'firmware_sha256':sha(blob),'app_sha256':sha(app.read_bytes()),
 'loader_sha256':sha(new_loader),'loader_source':'firmware/loader/loader.c; Felucca GPL-3.0-only',
 'app_bytes':app.stat().st_size,'app_headroom_bytes':581564-app.stat().st_size,
 'supported_flash_ids':['0x856014','0xB36014'],
 'source_loader_recompiled':True,'inherited_ota_loader_identical':False,
 'protected_16k_header_identical':True,'project_schema':'TRK1 V26',
 'stock_v15_sha256_pins_untouched':True,'hardware_tested':False,
 'uc25hq80_hardware_tested':False,'dma_deadlines_eliminated':False,
 'loader_hardware_tested':False,'readme':'README.md','changelog':'CHANGELOG.md',
 'validation':'V1.8.3-VALIDATION.md'}
for dest in [r/'FIRMWARE-MANIFEST.json',r/'BEETLE-V1.8.3-BUILD-MANIFEST.json']:
 dest.write_text(json.dumps(m,indent=2)+'\n')
print(json.dumps(m,indent=2))
