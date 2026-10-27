#!/usr/bin/env python3
"""Synchronize BEETLE V1.4.32 USB-C release assets, offline only."""
from pathlib import Path
import base64,hashlib,json,re
r=Path(__file__).resolve().parents[1]
fw=r/'BEETLE-V1.4.32-USB-AUDIO-STABILITY-EXPERIMENTAL.fwsc'
blob=fw.read_bytes();sha=hashlib.sha256(blob).hexdigest()
for name in ('BEETLE-Project-Manager.html','web/BEETLE-Project-Manager.html','simulator.html','web/simulator.html'):
 p=r/name
 s=p.read_text().replace('V1.4.31','V1.4.32')
 p.write_text(s)
p=r/'WebInstaller.html'
s=p.read_text().replace('V1.4.31 SID + BOOT KEY FIX','V1.4.32 USB AUDIO CAPTURE FIX').replace('V1.4.31','V1.4.32')
s=re.sub(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
s,n=re.subn(pat,lambda m:m[1]+'\n'+base64.b64encode(blob).decode()+'\n'+m[3],s,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pat,s,re.S)[2].strip())==blob
assert "HASH_ET: '"+sha+"'" in s
p.write_text(s);(r/'web/WebInstaller.html').write_text(s)
p=r/'BEETLE-MANAGER.html';manager=p.read_text().replace('V1.4.31','V1.4.32')
for tag,name in [('source-emulator','simulator.html'),('source-projectmanager','BEETLE-Project-Manager.html'),('source-installer','WebInstaller.html')]:
 q=rf'(<script id="{tag}" type="application/octet-stream">)(.*?)(</script>)'
 b=(r/name).read_bytes()
 manager,n=re.subn(q,lambda m:m[1]+base64.b64encode(b).decode()+m[3],manager,count=1,flags=re.S)
 assert n==1 and base64.b64decode(re.search(q,manager,re.S)[2])==b
p.write_text(manager)
manifest=r/'BEETLE-V1.4.32-BUILD-MANIFEST.json'
j=json.loads(manifest.read_text())
j.update({'version':'BEETLE V1.4.32','baseline':'BEETLE V1.4.31 protected header and OTA loader','usb_capture_ring_frames':512,
'usb_44100_initial_prime_frames':224,'usb_48000_initial_prime_frames':244,
'usb_capture_underrun_reprime':True,'usb_capture_overrun_reprime':True,'usb_capture_recovery_fades':True,
'usb_both_modes_slow_per_dma_clock_controller':True,'usb_stereo_or_stems_hardware_verified':False,
'webinstaller_embeds_firmware_sha256':sha,'hardware_validated':False})
manifest.write_text(json.dumps(j,indent=2)+'\n')
print('PASS: WebInstaller hash + embedded FWSC, All-in-One Manager embedded tools; firmware sha256:',sha)
