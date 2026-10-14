#!/usr/bin/env python3
"""Host-only safety checks for read-only flash atlas. No physical device needed."""
import importlib.util
import tempfile
from pathlib import Path
import subprocess,sys,json
root=Path(__file__).resolve().parents[2]
p=root/'tools/track_flash_audit.py'
spec=importlib.util.spec_from_file_location('track_flash_audit',p)
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
r=m.flash_map();assert r['storage_writes_allowed'] is False
assert r['track_owned_flash_bytes_verified']==0
assert r['required_track_journal_bytes']==262144
assert sum(x['size_bytes'] for x in r['regions'])==1048576
assert all(x['track_write_authorized'] is False for x in r['regions'])
try:m.flash_map(b'wrong')
except ValueError:pass
else:raise AssertionError('Accepted a truncated dump')
with tempfile.TemporaryDirectory() as d:
 d=Path(d)
 dump=d/'dump.bin';dump.write_bytes(bytes([0xFF])*0x100000)
 report=d/'report.json'
 cp=subprocess.run([sys.executable,str(p),'--dump',str(dump),'--json',str(report)],check=True,capture_output=True,text=True)
 data=json.loads(report.read_text())
 assert data['all_ff_sectors']==256
 assert data['storage_writes_allowed'] is False
 assert data['track_owned_flash_bytes_verified']==0  # blank does not mean owned
 assert data['full_dump_sha256']
 assert dump.read_bytes()==bytes([0xFF])*0x100000
 # Simulate Felucca project sector and OTA, must remain disallowed
 sample=bytearray(dump.read_bytes());sample[0x97000:0x97004]=b'FELU';sample[0xA0000:0xA0004]=b'FSMP';sample[0xE0000:0xE0004]=b'OTA!'
 dump.write_bytes(sample)
 audit=m.flash_map(sample)
 assert audit['storage_writes_allowed'] is False
 names={x['name']:x for x in audit['regions']}
 assert names['FELUCCA_PROJECTS_ETCH_STORAGE']['non_erased_bytes']>=4
 assert names['FELUCCA_SAMPLE_BANKS_ETCH_EXT']['non_erased_bytes']>=4
 assert names['OTA_STAGING']['non_erased_bytes']>=4
 try:subprocess.run([sys.executable,str(p),'--dump',str(dump),'--json',str(dump)],check=True,capture_output=True)
 except subprocess.CalledProcessError:pass
 else:raise AssertionError('Audit overwrote source dump')
 # Check the manifest doesn't silently ignore the upstream documented flash boundaries.
 f=(root/'reference/felucca-storage/fm1_flash.h').read_text()
 st=(root/'reference/felucca-storage/storage.c').read_text()
 assert '#define FL_DATA_LO      0x00097000u' in f
 assert '#define FL_GLOB_LO      0x000FC000u' in f
 assert '#define FL_OTA_LO       0x000E0000u' in f
 assert '0xE5000u + copy * ST_SECTOR' in st
 print('PASS: 1MiB atlas, exact-sector layout, zero implicitly owned flash, empty/foreign dumps, read-only input, source guard')
