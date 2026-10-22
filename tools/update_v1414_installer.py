#!/usr/bin/env python3
"""Bind V1.4.14 WebInstaller to the actual audited FWSC, not merely UI labels."""
from pathlib import Path
import base64,hashlib,re,sys
ROOT=Path(__file__).resolve().parents[1]
FWSC=ROOT/'BEETLE-V1.4.14-HUB-COMPAT-UNVERIFIED.fwsc'
sys.path.insert(0,str(ROOT/'tools'))
import fm1_fwsc as fw
binary=FWSC.read_bytes();actual,errs=fw.verify_fwsc(FWSC)
assert not errs and actual['product']=='FM-1_2123',errs
sha=hashlib.sha256(binary).hexdigest()
html=(ROOT/'WebInstaller.html').read_text()
old='BEETLE V1.4.13 REC LED / HUB DIAGNOSTICS'
new='BEETLE V1.4.14 HUB COMPATIBILITY LAB'
assert html.count(old)>12
html=html.replace(old,new).replace('V1.4.13 REC LED / HUB DIAGNOSTICS', 'V1.4.14 HUB COMPATIBILITY LAB')
html=html.replace('V1.4.13','V1.4.14')
html=html.replace('SikkenWorks BEETLE V1.4.14 HUB COMPATIBILITY LAB','SikkenWorks BEETLE V1.4.14 HUB COMPATIBILITY LAB')
pat=r"(HASH_ET:\s*')[0-9a-f]{64}(')"
assert len(re.findall(pat,html))==1
html=re.sub(pat,lambda m:m[1]+sha+m[2],html,count=1)
pat=r'(<script id="firmware-embedded" type="application/octet-stream">\s*)(.*?)(\s*</script>)'
assert len(re.findall(pat,html,flags=re.S))==1
html=re.sub(pat,lambda m:m[1]+base64.b64encode(binary).decode('ascii')+m[3],html,count=1,flags=re.S)
# Update ONLY the misleading legacy cross-firmware note, do not claim universal hub success.
html=html.replace('This firmware does not claim universal cross-firmware OTA compatibility, and the unknown-loader safety gate is unchanged.',
 'BEETLE now recognizes CRC-verified Felucca-family application-only loaders beyond its former SHA pin, but firmware switching via The Firmware Hub is NOT hardware verified. Official V15 remains exact-image-pinned; unknown loader families remain blocked.')
# Generic local installer deliberately keeps pinned-loader restrictions; the user asked for Hub support.
html=html.replace('The target package SHA-256, protected header and OTA-loader checks remain mandatory.',
 'The selected target and OTA loader still require validated compatibility; do not use this experimental installer as an unrestricted flasher.')
assert sha in html
assert base64.b64decode(re.search(pat,html,re.S).group(2).strip())==binary
for dest in [ROOT/'WebInstaller.html',ROOT/'web/WebInstaller.html']:
 dest.write_text(html)
print('Installer package',len(binary),'SHA256',sha, 'identity FM-1_2123')
