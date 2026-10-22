"""Offline installer / identity checks. NOT a claim of hardware OTA safety."""
from pathlib import Path
import base64
import hashlib
import importlib.util
import re
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import fm1_fwsc as fw
ref=ROOT/'reference/BEETLE-V1.3.8-ORIGINAL-PACKAGE.fwsc'
new=ROOT/'BEETLE-V1.4.8-STORAGE-OVERLAY-UNVERIFIED.fwsc'
pkg,errors=fw.verify_fwsc(new)
assert not errors and pkg['product']=='FM-1_2123'
assert fw.compare_packages(ref,new)['same_container_shape']
get=lambda file,key: next(item['data'] for item in fw.parse_ufw(file.read_bytes())['entries'] if item['name']==key)
assert get(ref,'flash.bin')[:0x4000]==get(new,'flash.bin')[:0x4000]
assert get(ref,'ota.bin')==get(new,'ota.bin')
app=(ROOT/'build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin').read_bytes()
newflash=bytearray(get(new,'flash.bin'))
assets=fw.extract_stock_assets(ref)
part=bytearray(newflash[fw.APP_AREA_FLASH:])
fw.sfc_xor(part,0,len(part),0,assets['key'])
assert part[0x120:0x120+len(app)]==app
html=(ROOT/'WebInstaller.html').read_text()
sha=hashlib.sha256(new.read_bytes()).hexdigest()
assert f"HASH_ET: '{sha}'" in html
assert f'ET_LENGTH: {new.stat().st_size}' in html
m=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',html,re.S)
assert m and base64.b64decode(m[1])==new.read_bytes()
assert 'BEETLE V1.4.8 STORAGE OVERLAY' in html
assert (ROOT/'web/WebInstaller.html').read_bytes()==(ROOT/'WebInstaller.html').read_bytes()
assert 'STORAGE FULL' in (ROOT/'BEETLE-Project-Manager.html').read_text()
print('V1.4.8 FWSC / WebInstaller SHA / V1.3.8 identity / boot + OTA loader parity: PASS (OFFLINE)')
