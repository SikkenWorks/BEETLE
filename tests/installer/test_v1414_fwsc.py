#!/usr/bin/env python3
"""Exact current release container + launcher parity, independent of old versions."""
from pathlib import Path
import base64,hashlib,json,re,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import fm1_fwsc as fw
ref=ROOT/'BEETLE-V1.4.5-PLAYHEAD-GHOST-FIX-UNVERIFIED.fwsc'
new=ROOT/'BEETLE-V1.4.14-HUB-COMPAT-UNVERIFIED.fwsc'
app=ROOT/'build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin'
report,errors=fw.verify_fwsc(new)
assert not errors,errors
assert report['product']=='FM-1_2123'
assert fw.compare_packages(ref,new)['same_container_shape']
a=fw.parse_ufw(ref.read_bytes());b=fw.parse_ufw(new.read_bytes())
get=lambda obj,name:next(item['data'] for item in obj['entries'] if item['name']==name)
assert get(a,'flash.bin')[:0x4000]==get(b,'flash.bin')[:0x4000]
assert get(a,'ota.bin')==get(b,'ota.bin')
assets=fw.extract_stock_assets(ref)
part=bytearray(get(b,'flash.bin')[fw.APP_AREA_FLASH:])
fw.sfc_xor(part,0,len(part),0,assets['key'])
manifest=json.loads((ROOT/'BEETLE-V1.4.14-BUILD-MANIFEST.json').read_text())
app_bytes=manifest['app_bytes']
embedded_app=bytes(part[0x120:0x120+app_bytes])
assert len(embedded_app)==app_bytes
assert hashlib.sha256(embedded_app).hexdigest()==manifest['app_sha256']
# A clean source checkout intentionally has no generated build/ directory.
# If rebuilt, additionally require exact bit-for-bit match to the new build.
if app.exists():
    assert embedded_app==app.read_bytes(), 'rebuilt JieLi app differs from packaged app' 
text=(ROOT/'WebInstaller.html').read_text()
assert (ROOT/'web/WebInstaller.html').read_bytes()==(ROOT/'WebInstaller.html').read_bytes()
sha=hashlib.sha256(new.read_bytes()).hexdigest()
assert f"HASH_ET: '{sha}'" in text
assert f'ET_LENGTH: {new.stat().st_size}' in text
m=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',text,re.S)
assert m and base64.b64decode(m[1])==new.read_bytes()
assert 'V1.4.14' in text and 'FM-1_2123' in text
print('PASS V1.4.14: V1.4.5 identity, protected header/OTA, app SHA against release manifest' + (', rebuilt app parity' if app.exists() else ', clean source') + ', exact installer embed/SHA')
