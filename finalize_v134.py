from pathlib import Path
import re,base64,json,hashlib,shutil
root=Path(__file__).resolve().parent
fw=root/'BEETLE-V1.3.4-USB-MULTI-FX-EXPERIMENTAL.fwsc'
src=root/'BEETLE-V1.3.3-USB-MULTITRACK-EXPERIMENTAL.fwsc'
oldhash=hashlib.sha256(src.read_bytes()).hexdigest()
newhash=hashlib.sha256(fw.read_bytes()).hexdigest()
installer=root/'WebInstaller.html'
s=installer.read_text()
s=s.replace('BEETLE V1.3.3','BEETLE V1.3.4').replace('BEETLE-V1.3.3','BEETLE-V1.3.4')
s=s.replace('FM-1_2118','FM-1_2119')
s=re.sub(r"HASH_ET: '[0-9a-f]{64}'","HASH_ET: '"+newhash+"'",s)
s,count=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)(.*?)(\s*</script>)',lambda m:m[1]+base64.b64encode(fw.read_bytes()).decode()+m[3],s,flags=re.S)
assert count==1 and oldhash not in s
installer.write_text(s)
(root/'web/WebInstaller.html').write_text(s)
(root/'web/simulator.html').write_bytes((root/'simulator.html').read_bytes())
readme=root/'README.md';s=readme.read_text().replace('Development release: <strong>V1.3.3</strong>','Development release: <strong>V1.3.4</strong>')
new='''**BEETLE V1.3.4** adds **up to three per-step FX** through PER STEP FX: FX1 remains the original effect; FX2 and FX3 are independent additional effects for the same step. In the FX page, K2 selects FX1/2/3 on synth lanes, K1 selects TYPE or VALUE and K3 changes the focused parameter. OCT+ moves to the next FX slot; OCT− removes the focused FX slot. On CH6, K2 switches NOTE1/NOTE2; use OCT+ for FX1/2/3. The tracker displays `MUL` when a step has multiple FX. FX1 must be present before FX2, which must be present before FX3. The project-wide pool has **48 extra FX entries** (FX2/FX3, not 48 per pattern). `FX STORAGE FULL` shows as an overlay when an insertion is rejected without overwriting existing events. Independent synth slots are processed in order; drum slots support the existing volume/pitch drum processing. The project codec is **V21** and reads earlier V19/V20 projects, but older firmware does not read V21. New projects may reach the existing 12,256-byte project-storage ceiling sooner. A complete backup and recovery path are essential before any hardware installation. Native code, WebSimulator and HELP contain the corresponding functions. Hardware audio and firmware flashing remain **unverified**.\n\n'''
s=s.replace('**BEETLE V1.3.3** adds',new+'**BEETLE V1.3.3** adds',1)
readme.write_text(s)
changelog=root/'CHANGELOG.md';s=changelog.read_text()
entry='''## V1.3.4 — Three Step FX Slots (experimental, 20 October 2026)\n\n- FX1 + FX2 + FX3 in the existing PER STEP FX menu. Additional effects use a project-wide, 48-record, bounded pool; no silent overwrites. `FX STORAGE FULL` overlay on rejected insertion, `MUL` label for multiple FX, separate CH6 NOTE1/NOTE2 values.\n- Synth DSP sequences primary and secondary effects; drums retain pitch and volume effects. FX edits, clearing lanes/ranges and full-pattern copy/paste include the additional FX and Undo/Redo state.\n- TRK1 V21 project format stores sparse extra FX and supports reading V19/V20. Older firmware cannot read V21. Existing 12,256-byte project-size cap is unchanged.\n- Source-level compilation for three JieLi variants, native audio and compatibility regressions, simulator syntax and package consistency tests. No physical FM1/USB/flash/recovery validation.\n\n'''
if not s.startswith('## V1.3.4'):changelog.write_text(entry+s)
(root/'docs/V1.3.4-VALIDATION.md').write_bytes((root/'V1.3.4-VALIDATION.md').read_bytes())
print('Installer, docs, simulator prepared; new FW sha',newhash)
