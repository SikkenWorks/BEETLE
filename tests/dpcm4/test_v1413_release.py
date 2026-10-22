"""V1.4.13 binary/installer audit, without any FM1 hardware I/O."""
from pathlib import Path
import base64,hashlib,re,sys
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools'))
import fm1_fwsc as fw
ref=root/'reference/BEETLE-V1.3.8-ORIGINAL-PACKAGE.fwsc'
new=root/'BEETLE-V1.4.13-REC-LED-HUB-LAB-UNVERIFIED.fwsc'
pkg,errors=fw.verify_fwsc(new)
assert not errors and pkg['product']=='FM-1_2123',errors
assert fw.compare_packages(ref,new)['same_container_shape']
get=lambda x,key: next(e['data'] for e in fw.parse_ufw(x.read_bytes())['entries'] if e['name']==key)
assert get(ref,'flash.bin')[:0x4000]==get(new,'flash.bin')[:0x4000]
assert get(ref,'ota.bin')==get(new,'ota.bin')
app=(root/'build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin').read_bytes()
part=bytearray(get(new,'flash.bin')[fw.APP_AREA_FLASH:]);assets=fw.extract_stock_assets(ref)
fw.sfc_xor(part,0,len(part),0,assets['key'])
assert part[0x120:0x120+len(app)]==app
html=(root/'WebInstaller.html').read_text()
sha=hashlib.sha256(new.read_bytes()).hexdigest()
assert f"HASH_ET: '{sha}'" in html
assert f'ET_LENGTH: {new.stat().st_size}' in html
match=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',html,re.S)
assert match and base64.b64decode(match[1])==new.read_bytes()
assert 'V1.4.13' in html
assert (root/'web/WebInstaller.html').read_bytes()==(root/'WebInstaller.html').read_bytes()
assert 'beetle_rec_led_on' in (root/'firmware/fm1-native/track_runtime.h').read_text()
assert 'FM-1_2123' in (root/'BEETLE-Hub-Diagnostics.html').read_text()
print('PASS V1.4.13: original product ID, loader/header parity, app binary, embedded FWSC and REC LED integration')
