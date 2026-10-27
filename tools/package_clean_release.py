#!/usr/bin/env python3
"""Create a cleaned complete BEETLE source distribution without old binaries/cache."""
import argparse, os, re, zipfile
from pathlib import Path
ap=argparse.ArgumentParser()
ap.add_argument('--version',required=True,help='e.g. V1.4.28')
ap.add_argument('--output',required=True)
a=ap.parse_args()
r=Path(__file__).resolve().parents[1]
out=Path(a.output).resolve()
assert re.fullmatch(r'V[0-9]+(?:\.[0-9]+)+',a.version),a.version
required=('README.md','CHANGELOG.md','LICENSE','BEETLE-MANAGER.html',
          'BEETLE-Project-Manager.html','WebInstaller.html','simulator.html')
for file in required:assert (r/file).is_file(),file
# Mandatory firmware on-device HELP must be synchronized with browser HELP.
import subprocess, sys
subprocess.run([sys.executable,str(r/'tests/track/test_help_contract.py')],check=True)
# Documentation contract: version-by-version narratives belong only in CHANGELOG.md.
readme=(r/'README.md').read_text(encoding='utf-8')
assert not re.search(r'^#{1,6}\s*(?:New in\s+V\d|V\d+(?:\.\d+){2,}\s*(?:—|[-:]))',readme,re.I|re.M), 'README includes version release notes; move them to CHANGELOG.md'
fw=list(r.glob(f'BEETLE-{a.version}-*.fwsc'))
assert len(fw)==1,fw
manifest=r/f'BEETLE-{a.version}-BUILD-MANIFEST.json'
assert manifest.exists()
roots={'firmware','tools','tests','docs','web','assets','LICENSES','reference','_includes'}
root_optional={'BEETLE.png','BEETLE-(banner).png','favicon.png',
 'BEETLE-Hub-Diagnostics.html','GLOBAL-BANK-STATUS.md','FIRMWARE-MANIFEST.json'}
selected=[]
for p in r.rglob('*'):
 if not p.is_file() or p.resolve()==out:continue
 rel=p.relative_to(r); parts=rel.parts
 if '.git' in parts or 'build' in parts or '__pycache__' in parts:continue
 if p.suffix.lower() in {'.pyc','.o','.elf','.bin','.log','.zip','.tar','.xz'}:continue
 if any(part.startswith('.') for part in parts):continue
 if len(parts)==1:
  if p.name not in required and p.name not in root_optional and p!=fw[0] and p!=manifest:continue
 elif parts[0] not in roots:continue
 selected.append((str(rel).replace(os.sep,'/'),p))
selected.sort()
with zipfile.ZipFile(out,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=7) as z:
 for name,p in selected:z.write(p,name)
with zipfile.ZipFile(out) as z:
 bad=z.testzip();assert bad is None,bad
 assert len(z.namelist())==len(selected)
 assert not any('V1.4.27' in n and n.startswith('BEETLE-') for n in z.namelist())
print(f'Clean BEETLE {a.version}: {len(selected)} files; {out.stat().st_size:,} bytes; {out}')
