#!/usr/bin/env python3
"""Keep only latest generated artifacts without touching authored source or tests.
Run from a new BEETLE-V* release directory: python source/tools/clean_release.py --apply
Dry-run is default. It NEVER accesses the FM1 or external drives.
"""
from pathlib import Path
import argparse,shutil
ap=argparse.ArgumentParser()
ap.add_argument('--apply',action='store_true')
a=ap.parse_args()
root=Path(__file__).resolve().parents[2]
if not root.name.startswith('BEETLE-V0.') or not (root/'source/firmware/fm1-native/build.py').exists():
 raise SystemExit('Refusing to clean outside a BEETLE source release')
version=root.name[len('BEETLE-'):]
paths=[]
for p in root.iterdir():
 if p.is_dir() and p.name.startswith('previous-release-'):paths.append(p)
 if p.is_file() and p.name.startswith('BEETLE-V0.') and version not in p.name:paths.append(p)
for p in (root/'source/web').glob('BEETLE-V0.*'):
 if version not in p.name:paths.append(p)
for p in (root/'source/tools').glob('beetle_v0*_storage_pack.py'):
 if version.replace('.','').lower() not in p.name:paths.append(p)
for p in (root/'source/tools').glob('embed_v0*_webinstaller.py'):
 if version.replace('.','').lower() not in p.name:paths.append(p)
for p in (root/'source/build').rglob('*'):
 if p.is_file() and (p.suffix in {'.o','.elf','.log','.tmp'}):paths.append(p)
for p in (root/'source').rglob('__pycache__'):
 if p.is_dir():paths.append(p)
seen=set()
for p in sorted(paths,key=lambda f:len(f.parts),reverse=True):
 if p in seen or not p.exists():continue
 seen.add(p);print(('REMOVE' if a.apply else 'WOULD REMOVE'),p.relative_to(root))
 if a.apply:
  if p.is_dir():shutil.rmtree(p)
  else:p.unlink()
print(f'{len(seen)} cleanup paths; '+('applied' if a.apply else 'dry-run'))
