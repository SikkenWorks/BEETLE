#!/usr/bin/env python3
"""Preserve original layouts/assets, update release metadata and embedded binaries."""
from pathlib import Path
import base64,hashlib,re,shutil,json,subprocess,sys
r=Path(__file__).resolve().parents[1]
v='V1.4.27'
fw=r/'BEETLE-V1.4.27-CRY-BITCRUSH-RADIO-EXPERIMENTAL.fwsc'
blob=fw.read_bytes();sha=hashlib.sha256(blob).hexdigest()
# Exact complete simulator, retaining all functions/graphics.
sim=(r/'simulator.html').read_text()
assert "'CRY'" in sim and "'SCRATCH'" not in sim
sim=sim.replace('V1.4.26',v)
for f in ('simulator.html','web/simulator.html','BEETLE-V1.4.27-WebSimulator.html'):(r/f).write_text(sim)
# Installer: update only bundled binary, SHA pin and *presentation* metadata.
inst=(r/'WebInstaller.html').read_text()
assert len(re.findall(r"HASH_ET: '[0-9a-f]{64}'",inst))==1
inst=inst.replace('V1.4.26 AUDIO FX + NOTE2 (EXPERIMENTAL)','V1.4.27 CRY / BITCRUSH / RADIO (EXPERIMENTAL)').replace('V1.4.26',v)
inst=re.sub(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],inst,count=1)
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
inst,n=re.subn(pat,lambda m:m[1]+'\n'+base64.b64encode(blob).decode()+'\n'+m[3],inst,count=1,flags=re.S)
assert n==1 and base64.b64decode(re.search(pat,inst,re.S).group(2).strip())==blob
assert sha in inst
for f in ('WebInstaller.html','web/WebInstaller.html','BEETLE-V1.4.27-WebInstaller.html'):(r/f).write_text(inst)
# Standalone manager code unchanged, no dropped import/export/transfer features.
shutil.copy2(r/'BEETLE-Project-Manager.html',r/'BEETLE-V1.4.27-Project-Manager.html')
readme=r/'README.md';s=readme.read_text()
assert '## New in V1.4.26:' in s
s=s.replace('Development release: <strong>V1.4.26 EXPERIMENTAL</strong>','Development release: <strong>V1.4.27 EXPERIMENTAL</strong>',1)
intro='''## New in V1.4.27: CRY, Audible BITCRUSH & RADIO Low-Cut

**CRY** is the new name for the former **SCRATCH** Punch-In effect (FX 09). Its effect ID, held-key mapping, captured performance automation and parameters are unchanged. It is still short-buffer, bidirectional scratch playback rather than a long, full reverse track. The firmware Punch-In pages, recorded FX event editor and WebSimulator display **CRY**.

**BITCRUSH (FX 10)** now combines real **12-to-4-bit quantization** with **sample-rate reduction (44.1 kHz down to approximately 1.4 kHz at maximum)**. Hold its white key and turn **K1**: at 50 the native engine produces roughly **8-bit / 2.76 kHz** processing, deliberately more audible on quiet patches and drums than the previous amplitude-only version. State persists across DMA blocks, while short wet/dry fades avoid abrupt clicks. This is deliberately aggressive digital degradation at higher settings.

**RADIO (FX 15)** now uses **two high-pass stages** for a clearly audible low-cut, preserving upper frequencies without a telephone-style low-pass. K1 raises the bass cutoff and gradually increases a small stereo hiss component. The source music is retained; this is **not** noise-only playback. A short transition fades the effect in/out.

**Verification:** a new host-native DSP regression checks quiet-signal BITCRUSH, 200 Hz vs 3 kHz RADIO attenuation, and that idle hiss remains low. It also checks NOTE2 / sample compatibility from V1.4.26. **The physical FM1, live CPU deadline, USB audio and OTA recovery still need real-device validation**. Back up before flashing; a successful native build does not certify the hardware.

'''
s=s.replace('## New in V1.4.26:',intro+'## New in V1.4.26:',1)
for f in ('README.md','BEETLE-V1.4.27-README.md'):(r/f).write_text(s)
ch=r/'CHANGELOG.md';txt=ch.read_text()
release='''## V1.4.27 — Punch-In BITCRUSH/RADIO DSP & CRY Renaming (experimental)

- Renamed Punch-In FX 09 from SCRATCH to **CRY**, including native UI, Punch-In event editor, simulator and README. FX ID 8 (zero-based) and project schema **V25** remain unchanged.
- BITCRUSH now combines 12–4-bit sample quantization with a continuous stateful sample-and-hold reducer (1–31 PCM frames at 44.1 kHz), preventing the formerly subtle bit-depth-only effect. Default K1 50 is approximately 8 bits at 2.76 kHz effective update rate.
- RADIO now applies two cascaded highpass stages (bass/low-cut) with subtle independent stereo hiss; upper frequencies remain present and the dry signal is never replaced by hiss. Short fades tame key-on transients. No additional PCM history allocation.
- Added native frequency-response, low-amplitude BITCRUSH and noise-level regression tests, with UndefinedBehaviorSanitizer; V1.4.26 NOTE2 and FX tests retained.
- JieLi native USB / Storage V2 build succeeded. Package preserves the protected header, original OTA loader and `FM-1_2123` identity. Hardware playback and OTA/recovery validation remain outstanding.

'''
if '## V1.4.27' not in txt:
 idx=txt.find('## V1.4.26')
 assert idx>=0
 txt=txt[:idx]+release+txt[idx:]
ch.write_text(txt)
(r/'docs/V1427-CRY-BITCRUSH-RADIO.md').write_text('''# BEETLE V1.4.27 Punch-In FX testing

This release changes only CRY labeling and the native and simulator BITCRUSH and RADIO DSP. Project format remains TRK1 V25; FX IDs and all 13 global sample labels/storage behavior stay unchanged.

## Manual physical FM1 acceptance test

1. Back up `.beetle` projects and the `.beetlebank`. Confirm a genuinely working recovery route **before** experimental flashing.
2. On FX page 2, hold the former SCRATCH key: it must now display **CRY**, with identical bidirectional sound and adjustable K1 setting. Recorded FX must show CRY in the event editor.
3. Test BITCRUSH on a bassline, a quiet sustained high note, and drums. Hold FX 10. K1 50 should give an unmistakably granular/downsampled tone; turning up should audibly increase degradation. Releasing should restore dry audio without clicks.
4. Test RADIO on a bass-heavy pattern. Hold FX 15: the bottom end should be cut while melody and upper percussion are retained with light hiss. K1 should increase lowcut and hiss. Releasing must restore bass.
5. Test 16 FX switching and last-held-key page focus, NOTE2 U01–U13, six-note polyphony, long song loops, full MASTER FX and USB multitrack for clipping, DMA misses, stuck sound and glitches.
6. Test successful OTA update and return using a separately verified recovery path. Passing software package hashes is not hardware OTA validation.

## Native regression

```bash
bash tests/track/run_v1427_fx.sh
```

Expected: audible PCM changes at low input amplitude, RADIO 200 Hz attenuation stronger than 3 kHz, stable low-level noise and V1.4.26 NOTE2 legacy codec pass.

## Constraints

- Shared 12 KiB Punch-In history unchanged; no new persistent PCM buffers.
- Bitcrush state: two held PCM values, one counter, one crossfade value.
- Radio reuses existing per-channel highpass state with one crossfade byte.
- Storage V2 layout, project serialization V25 and loader unchanged.
- No physical FM1 DMA, acoustic or real USB tests have been performed here.
''')
print('Updated simulator, installer, README, changelog, doc; SHA=',sha)
subprocess.run([sys.executable,str(r/'tools/build_all_in_one_manager.py'),str(r/'BEETLE-MANAGER.html'),str(r/'BEETLE-V1.4.27-WebSimulator.html'),str(r/'BEETLE-V1.4.27-Project-Manager.html'),str(r/'BEETLE-V1.4.27-WebInstaller.html'),str(r/'BEETLE-V1.4.27-MANAGER.html')],check=True)
shutil.copy2(r/'BEETLE-V1.4.27-MANAGER.html',r/'BEETLE-MANAGER.html')
print('All-in-one bundle updated.')
