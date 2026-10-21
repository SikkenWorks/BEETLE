#!/usr/bin/env python3
"""Regenerate BEETLE-MANAGER.html with the exact three standalone tools.

Run on EVERY release, AFTER generating simulator.html, Project Manager,
and WebInstaller.html (with its target firmware embedded). This uses the
same-origin srcdoc manager template; never reintroduce blob navigation
for the embedded tools. -- Python standard library only.
"""
from pathlib import Path
import base64,re,sys,hashlib
if len(sys.argv)!=6:
 raise SystemExit('usage: build_all_in_one_manager.py TEMPLATE.html SIMULATOR.html PROJECT_MANAGER.html WEBINSTALLER.html OUTPUT.html')
template,sim,projects,installer,out=map(Path,sys.argv[1:])
version=next((m.group(0) for name in (sim.name,installer.name) if (m:=re.search(r'V\d+\.\d+\.\d+',name))),None)
if not version:raise SystemExit('Use a versioned simulator or installer filename like BEETLE-V1.3.9-simulator.html')
html=template.read_text(encoding='utf-8')
if 'frame.srcdoc=base64ToUtf8' not in html or 'frame.src=documentUrl(id)' in html:raise SystemExit('Unsafe/outdated template: srcdoc is required for embedded MIDI tools')
for label,path in [('emulator',sim),('projectmanager',projects),('installer',installer)]:
 pattern=fr'(<script id="source-{label}" type="application/octet-stream">)[\s\S]*?(</script>)'
 payload=base64.b64encode(path.read_bytes()).decode('ascii')
 html,count=re.subn(pattern,lambda match:match[1]+payload+match[2],html,count=1)
 if count!=1:raise SystemExit(f'Missing source slot for {label}')
 assert base64.b64decode(re.search(pattern,html).group(0).split('>',1)[1].split('</script>',1)[0])==path.read_bytes()
 print(f'{label}: {len(path.read_bytes())} bytes; SHA-256 {hashlib.sha256(path.read_bytes()).hexdigest()}')
# Only replace version labels in outer document; base64 is unchanged.
html=re.sub(r'V\d+\.\d+\.\d+',version,html)
out.write_text(html,encoding='utf-8')
print('Built',out,version,len(html))
