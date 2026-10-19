#!/usr/bin/env python3
"""Offline exact UFW payload fixtures; never contacts the FM1."""
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools'))
import fm1_fwsc as fw
src=(root/'BEETLE-1.01-RC1.0.9-EXPERIMENTAL.fwsc').read_bytes()
base=fw.parse_ufw(src)
flash=base['entries'][0]['data'];loader=base['entries'][1]['data']
out=root/'tests/ota/generated';out.mkdir(exist_ok=True)
def save(label,f,l):
  raw=fw.build_fwsc(bytes(f),bytes(l),'FM-1_2109')
  package=fw.parse_ufw(raw)
  (out/(label+'.logical.bin')).write_bytes(fw.decode_fwsc(raw)[1]);(out/(label+'.fwsc')).write_bytes(raw)
  return package
save('beetle_rc109',flash,loader)
assets=fw.extract_stock_assets(root/'reference/BEETLE-RC1.0.5-PROTECTED-BOOT-REFERENCE.fwsc.reference')
# Decrypt the packaged padded app. No previously compiled build folder required.
enc=bytearray(flash[0x4000:0x4000+0x120+fw.APP_SLOT])
fw.sfc_xor(enc,0,len(enc),0,fw.DEFAULT_CHIP_KEY)
app=bytes(enc[0x120:])
alt=fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload'])
changed=[i for i in range(0x4000) if alt[i]!=flash[i]]
assert changed==[0x20,0x21,0x22,0x23],changed[:20]
f=bytearray(flash);f[:0x4000]=alt[:0x4000]
assert fw.parse_flash(f)['app_crc_ok']
save('compatible_foreign_head',f,loader)
# Corrupt a ciphered app-byte but reconstruct valid outer UFW CRC.
f=bytearray(flash);f[0x4000+0x120+777]^=0x18
save('corrupt_app_payload',f,loader)
# Wrong FM1 XIP address, proper JLFS header CRC.
f=bytearray(flash);region=bytearray(f[0x4000:0x4000+0x20]);fw.sfc_xor(region,0,len(region),0,fw.DEFAULT_CHIP_KEY)
region[4:8]=(0x03000120).to_bytes(4,'little');region[:2]=fw.crc16(region[2:]).to_bytes(2,'little')
fw.sfc_xor(region,0,len(region),0,fw.DEFAULT_CHIP_KEY);f[0x4000:0x4020]=region
save('wrong_app_xip',f,loader)
# Untrusted loader; the incoming app is valid, but never execute its loader.
l=bytearray(loader);l[0x20+90]^=1
# Recompute both loader body-CRC and outer header-CRC so only SHA pin rejects.
inner_crc=fw.crc16(l[0x20:]);l[2:4]=inner_crc.to_bytes(2,'little');l[:2]=fw.crc16(l[2:32]).to_bytes(2,'little')
save('untrusted_loader',flash,l)
# Original ETCH fixtures are checked into the source; decode them for native replay.
for original in [root/'tests/ota/fixtures/ETCH-V1.36-ORIGINAL.fwsc',root/'reference/ETCH-FM1-V1.37-ORIGINAL.fwsc']:
  (out/(original.stem+'.logical.bin')).write_bytes(fw.decode_fwsc(original.read_bytes())[1])
print('generated files', len(list(out.glob('*.logical.bin'))),'changed header bytes',changed)
