#!/usr/bin/env python3
"""Refresh project-author 3-in-1 template from exact matching three standalone HTML files.

Source template stays embedded in the repo; no network, no external dependencies.
"""
from pathlib import Path
import base64,re,sys,hashlib
if len(sys.argv)!=6:
 raise SystemExit('usage: build_all_in_one_manager.py TEMPLATE.html SIMULATOR.html PROJECT_MANAGER.html WEBINSTALLER.html OUTPUT.html')
template,simulator,projectmanager,installer,output=map(Path,sys.argv[1:]);html=template.read_text('utf-8')
assert '<title>BEETLE · Emulator / Project Manager / WebInstaller' in html
for label,src in [('emulator',simulator),('projectmanager',projectmanager),('installer',installer)]:
 p=r'(<script id="source-'+label+r'" type="application/octet-stream">)[\s\S]*?(</script>)'
 payload=base64.b64encode(src.read_bytes()).decode('ascii')
 html,n=re.subn(p,lambda m: m[1]+payload+m[2],html,count=1)
 assert n==1,f'missing embedded {label}'
 match=re.search(p,html)
 assert match and base64.b64decode(match[0].split('>',1)[1].split('</script>')[0])==src.read_bytes()
 print(f'{label}: {len(src.read_bytes())} bytes sha256 {hashlib.sha256(src.read_bytes()).hexdigest()}')
html=re.sub(r'V1\.3\.[0-9]+', 'V1.3.8', html)
assert html.count('V1.3.8')>=3
output.write_text(html,'utf-8')
print('ALL-IN-ONE',output,len(html))
