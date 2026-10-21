#!/usr/bin/env python3
from pathlib import Path
import json, hashlib, sys, subprocess
root=Path(__file__).resolve().parents[2]
out=root/'build/track-fm1-native-test'
app=out/'app.bin'
manifest=json.loads((out/'native-build-options.json').read_text())
report=json.loads((out/'app.bin.audit.json').read_text())
assert app.is_file() and len(app.read_bytes())>0
assert report['valid'] and report['entry_stub_ok']
assert len(app.read_bytes())==149400
assert manifest['version']=='TRACK-0.8'
assert manifest['flash_read_only'] and manifest['readonly_hardware_test']
assert not manifest['storage_writes'] and not manifest['ota_writes']
assert hashlib.sha256(app.read_bytes()).hexdigest()==report['app_sha256']==manifest['app_sha256']
sections={i['name']:i for i in report['sections']}
assert sections['.bss']['size']<=96*1024
assert sections['.pool']['size']<=0x54000
assert sections['.noinit']['size']<=0x3d50
print('TRACK V0.8: linked binary, entry, SHA-256, read-only guards, RAM/POOL/NOINIT PASS')
