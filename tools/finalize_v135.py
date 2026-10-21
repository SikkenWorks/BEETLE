from pathlib import Path
import re,base64,hashlib,json
root=Path(__file__).resolve().parents[1]
firmware=root/'BEETLE-V1.3.5-USB-MULTI-FX-EXPERIMENTAL.fwsc'
raw=firmware.read_bytes();sha=hashlib.sha256(raw).hexdigest()
manifest=json.loads((root/'FIRMWARE-MANIFEST.json').read_text())
assert sha==manifest['sha256']
s=(root/'WebInstaller.html').read_text()
s=s.replace('BEETLE V1.3.4','BEETLE V1.3.5').replace('BEETLE-V1.3.4','BEETLE-V1.3.5')
s=s.replace("ID_ET: 'FM-1_2119'","ID_ET: 'FM-1_2120'")
s,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'","HASH_ET: '"+sha+"'",s)
assert n==1,n
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)(.*?)(\s*</script>)',lambda m:m[1]+base64.b64encode(raw).decode()+m[3],s,flags=re.S)
assert n==1
(root/'WebInstaller.html').write_text(s)
(root/'web/WebInstaller.html').write_text(s)
for x in (root/'simulator.html',root/'web/simulator.html',root/'BEETLE-V1.3.5-Workbench.html'):
 assert x.read_bytes()==(root/'simulator.html').read_bytes(),x
(root/'docs/V1.3.5-VALIDATION.md').write_text('''# BEETLE V1.3.5 Validation (software only)\n\n- New user-supplied `simulator(2).html` layout retained as `simulator.html`, with the embedded native-equivalent engine updated.\n- FX1, FX2, FX3 are independent: K1 selects row; K2 changes type; K3 value; OCT− removes only selected effect.\n- Project format upgraded to V22 because FX2/3 slot identities must be stored explicitly. V21/V20/V19 input supported. V22 projects cannot be opened in V1.3.4.\n- Fx pool capacity remains 48 total extra effect records per project.\n- JieLi firmware app built for storage/OTA lab, OTA pilot and read-only diagnostics.\n- App image and container structurally verified; V1.3.4 protected header and OTA loader preserved byte for byte.\n- All 27 physical keyboard positions tested in browser for F3–G5, with contextual SEQ OP/mixer behavior.\n- Host C, JS/Node, Chromium Playwright and firmware/installer checks: see tests/track/run_v135_regression.sh.\n- No physical M-VAVE FM1 testing, real USB/DAW operation, firmware flashing or recovery validation. This is experimental firmware, not a safety certification.\n''')
(root/'V1.3.5-VALIDATION.md').write_bytes((root/'docs/V1.3.5-VALIDATION.md').read_bytes())
print('WebInstaller synced, sha',sha)
