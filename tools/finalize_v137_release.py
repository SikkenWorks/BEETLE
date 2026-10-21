#!/usr/bin/env python3
"""Synchronize native V1.3.7, user-supplied 3-in-1 manager, and three individual HTML tools."""
from pathlib import Path
import base64,hashlib,json,re,shutil
root=Path(__file__).resolve().parents[1]
firmware=root/'BEETLE-V1.3.7-USB-MULTI-FX-EXPERIMENTAL.fwsc'
manifest=json.loads((root/'FIRMWARE-MANIFEST.json').read_text())
assert hashlib.sha256(firmware.read_bytes()).hexdigest()==manifest['sha256']
# Reuse latest V1.3.6 installer and replace firmware data; ETCH/stock/recovery payloads unchanged.
inst=(root/'web/templates/BEETLE-WebInstaller-V1.3.6.html').read_text()
assert inst.count("ID_ET: 'FM-1_2121'")==1
inst=inst.replace('V1.3.6','V1.3.7').replace("ID_ET: 'FM-1_2121'","ID_ET: 'FM-1_2122'")
inst,n=re.subn(r"HASH_ET: '[a-f0-9]{64}'","HASH_ET: '"+manifest['sha256']+"'",inst,count=1)
assert n==1
inst,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)(.*?)(\s*</script>)',lambda m:m[1]+base64.b64encode(firmware.read_bytes()).decode()+m[3],inst,count=1,flags=re.S)
assert n==1
for name in ('WebInstaller.html','BEETLE-V1.3.7-WebInstaller.html','web/WebInstaller.html'):
 (root/name).write_text(inst)
# Author's Workbench retained byte-for-byte except embedded native-version labels from earlier source preparation.
workbench=(root/'simulator.html').read_text()
assert 'V1.3.7' in workbench and 'id="beetle-engine"' in workbench
for name in ('simulator.html','BEETLE-V1.3.7-simulator.html','BEETLE-V1.3.7-Workbench.html','web/simulator.html'):
 (root/name).write_text(workbench)
proj=(root/'BEETLE-Project-Manager.html').read_text().replace('V1.3.6','V1.3.7').replace('V1.3.5','V1.3.7').replace('V1.3.4','V1.3.7').replace('V1.2 simulator','BEETLE simulator')
for name in ('BEETLE-Project-Manager.html','BEETLE-V1.3.7-Project-Manager.html'):
 (root/name).write_text(proj)
# Exact user-supplied, self-contained 3-in-1 file is the master for its structure and CSS.
user_file=root/'web/templates/BEETLE-MANAGER.user-v135.html'
manager=user_file.read_text()
for id,source in (('emulator',workbench),('projectmanager',proj),('installer',inst)):
 pat=r'(<script id="source-'+id+r'" type="application/octet-stream">)(.*?)(</script>)'
 manager,n=re.subn(pat,lambda m:m[1]+base64.b64encode(source.encode()).decode()+m[3],manager,count=1,flags=re.S)
 assert n==1,id
manager=manager.replace('V1.3.5','V1.3.7').replace('V1.3.6','V1.3.7')
# Global shared no-selection/contextmenu restriction, incl. manager shell.
assert 'selectstart' in manager or 'user-select:none' in manager
for name in ('BEETLE-MANAGER.html','BEETLE-V1.3.7-MANAGER.html'):
 (root/name).write_text(manager)
# No stale release-named top-level files from older versions beyond explicit archive sources.
for old in ('BEETLE-V1.3.6-WebInstaller.html','BEETLE-V1.3.6-Project-Manager.html'):
 (root/old).unlink(missing_ok=True)
# README/changelog: preserve supplied README as authoritative, only append release-specific reference.
rp=root/'README.md';readme=rp.read_text()
addition='''\n### V1.3.7 native FM1 SETTINGS & All-in-One Manager\n\n- **Physical FM1 controls:** Open `HOME > PROJECTS`, turn K1 to **SETTINGS**, and press the **SEL button**. The native input transaction explicitly consumes this keypress; the USB toggle is armed only after SEL is released. Press SEL again to switch 2-channel stereo ↔ 6-channel mono capture, then reconnect/reopen the host USB audio device. HOME returns to PROJECTS. This is a firmware change, not just an emulator fix.\n- `BEETLE-MANAGER.html` is the **offline all-in-one** containing this release's actual simulator, Project Manager, and WebInstaller. The three standalone `.html` tools are shipped separately as well.\n- The simulator is a software preview, **not proof** that physical FM1 USB re-enumeration and DAW multitrack input work. Firmware flashing and recoverability require testing on a physical device with a proven recovery path.\n'''
assert 'BEETLE-MANAGER.html' in readme or 'V1.3.7' in readme
readme=readme.replace('## Downloads','## Downloads',1) # no layout or credits changes
if '### V1.3.7 native FM1 SETTINGS' not in readme:readme+='\n'+addition
rp.write_text(readme)
cp=root/'CHANGELOG.md';changelog=cp.read_text();tag='### V1.3.7 physical FM1 SEL/SETTINGS fix'
if tag not in changelog:
 changelog=changelog.replace('## V1.3.7',tag+'\n\n- Implemented a dedicated native physical SEL button transaction. SETTINGS entry now consumes the select edge, has a trailing-edge fallback for an interrupted poll, and blocks USB toggles until a separate SEL press after release.\n- Added a host regression for physical matrix bit 3, short press, slow press, release, duplicate prevention, and unrelated PROJECTS/SEQ/HOME navigation.\n- The exact user-provided BEETLE-MANAGER.html design now embeds V1.3.7 versions of all three tools.\n\n## V1.3.7',1)
 cp.write_text(changelog)
# Guard no regression in historic code links while providing latest aliases.
print(json.dumps({'firmware_sha256':manifest['sha256'],'firmware_bytes':firmware.stat().st_size,'manager_bytes':(root/'BEETLE-MANAGER.html').stat().st_size,'simulator_bytes':(root/'simulator.html').stat().st_size,'installer_bytes':(root/'WebInstaller.html').stat().st_size,'project_manager_bytes':(root/'BEETLE-Project-Manager.html').stat().st_size},indent=2))
