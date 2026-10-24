#!/usr/bin/env python3
"""Rebind the verified FM1 package and preserve the established README style."""
from pathlib import Path
import hashlib, base64, re
ROOT=Path(__file__).resolve().parents[1]
VERSION='V1.4.26'
FW=ROOT/'BEETLE-V1.4.26-FX-AUDIO-NOTE2-EXPERIMENTAL.fwsc'
blob=FW.read_bytes(); sha=hashlib.sha256(blob).hexdigest()
# Never change WebInstaller's OTA/check structure: update only pinned package, title, SHA.
old=(ROOT/'WebInstaller.html').read_text(encoding='utf-8')
assert len(re.findall(r"HASH_ET: '[0-9a-f]{64}'", old))==1
new=old.replace('V1.4.25 PUNCH-IN FX2 + PROJECT RESERVE (EXPERIMENTAL)',
                'V1.4.26 AUDIO FX + NOTE2 (EXPERIMENTAL)').replace('V1.4.25',VERSION)
new=re.sub(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],new,count=1)
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)(.*?)(</script>)'
new,n=re.subn(pat,lambda m:m[1]+'\n'+base64.b64encode(blob).decode('ascii')+'\n'+m[3],new,count=1,flags=re.S)
assert n==1
assert len(re.findall(r"HASH_ET: '[0-9a-f]{64}'",new))==1
assert base64.b64decode(re.search(pat,new,re.S)[2].strip())==blob
for p in (ROOT/'WebInstaller.html', ROOT/'web/WebInstaller.html',ROOT/f'BEETLE-{VERSION}-WebInstaller.html'): p.write_text(new,encoding='utf-8')
# Current simulator already edited with native NOTE2/FX; update only visible release labels.
sim=(ROOT/'simulator.html').read_text(encoding='utf-8')
sim=sim.replace('V1.4.25',VERSION)
for p in (ROOT/'simulator.html',ROOT/'web/simulator.html',ROOT/f'BEETLE-{VERSION}-WebSimulator.html'): p.write_text(sim,encoding='utf-8')
# Keep the full original README markup, badges, existing tables and credits. Insert small release note.
readme=(ROOT/'README.md').read_text(encoding='utf-8')
assert readme.count('## What is BEETLE?')==1
readme=readme.replace('Development release: <strong>V1.4.25 EXPERIMENTAL</strong>',
                       'Development release: <strong>V1.4.26 EXPERIMENTAL</strong>',1)
anchor='## New in V1.4.25: Sixteen Punch-In FX & Custom Sample Labels'
assert anchor in readme
release='''## New in V1.4.26: Punch-In DSP, User Sample Looping & NOTE2

**Audio effects corrected:** SCRATCH now makes a fast back-and-forth movement through the existing short stereo history, with forward and reverse playback speed controlled by K1. BITCRUSH always reduces the effective bit depth rather than reaching an inaudible 16-bit setting. PHASER now cascades four all-pass stages and mixes wet/dry for moving phase notches. RADIO applies a distinct voice-band EQ with subdued noise, and DJ ISOLATOR uses separate bass, mid and treble components for audible band kills. The other Punch-In FX, both pages and automatic last-held-key focus remain intact.

**Global U01–U13 samples on CH6 NOTE2:** NOTE2 now accepts **all 14 factory drums plus all 13 user samples**, just like NOTE1. Use **K2** to focus NOTE2 and **K3** to scroll through factory and U01–U13 choices, or press a drum key with Tracker REC armed to write into NOTE2. NOTE1 remains unaffected. NOTE2 keeps separate step FX. Existing NOTE2 factory hits, STOP commands, and TRK1 **V25** projects continue to decode; no new project format is required.

**Looping sample stability:** the native Storage V2 BD4S streamer now reuses previously verified sectors and avoids redundant flash reads when up to six drumvoices retrigger in short patterns. This reduces unnecessary flash traffic without increasing audio buffer size, changing the 22,050 Hz / 4-bit sample format, or reducing synth polyphony. The WebSimulator reflects NOTE2 editing but does not have direct access to global samples stored on a physical FM1.

**Firmware safety:** this is an experimental source and software-tested firmware update. The protected 16 KiB header, original OTA loader, device identity `FM-1_2123`, Storage V2 protocol and sample/project reserve are retained. **Actual FM1 sound, USB multitrack stability, sustained DSP load, flash update success and hardware recovery are not yet validated.** Export project/sample backups before any update.

'''
if '## New in V1.4.26:' not in readme: readme=readme.replace(anchor,release+anchor,1)
readme=readme.replace('Press **K2** to focus **NOTE2** and **K3**', 'Press **K2** to focus **NOTE2** and **K3**')
# Targeted existing references instead of replacing historical version references.
readme=readme.replace('The original 14 factory hits are retained;', 'The original 14 factory hits are retained;')
readme=readme.replace('**Tracker / CH6** | With REC armed, enter a drum in NOTE1, or a separate second hit when **NOTE2** is focused.',
                      '**Tracker / CH6** | With REC armed, enter a drum in NOTE1, or a separate second hit (factory drum or U01–U13) when **NOTE2** is focused.')
for p in (ROOT/'README.md',ROOT/f'BEETLE-{VERSION}-README.md'):p.write_text(readme,encoding='utf-8')
ch=ROOT/'CHANGELOG.md'; c=ch.read_text(encoding='utf-8')
cnew='''## V1.4.26 — Punch-In DSP Corrections, NOTE2 User Samples & Reduced Flash Traffic (experimental)

- Reworked SCRATCH's short-buffer forward/reverse transport, improved BITCRUSH resolution, corrected 4-stage PHASER, tuned RADIO band-pass/hiss and DJ ISOLATOR bass/mid/treble cuts. Fixed recovery of all 16 Punch-In parameter values.
- NOTE2 now supports all fourteen factory drums and thirteen user samples. A formerly unused bit in the drum KIT field extends NOTE2 IDs without adding bytes to TRK1 V25, changing NOTE1 or breaking legacy NOTE2 data. Native runtime and WebSimulator use the same encoding.
- Improved Storage V2 global sample streaming: skips redundant flash reads, reuses one verified sector across voices and maintains per-voice generation validation. Six-voice native host-streaming tests and 27-NOTE2-value project codec tests pass.
- JieLi native build with UAC/Storage V2 passes, firmware container verified with protected header and OTA loader unchanged. **The physical FM1 DMA deadline, sound quality, USB recording and recovery have NOT been validated.** Back up project/sample data before upgrading.

'''
# Maintain changelog original intro (if any).
idx=c.find('## V1.4.25')
assert idx>=0
if '## V1.4.26' not in c: c=c[:idx]+cnew+c[idx:]
ch.write_text(c,encoding='utf-8')
print('Pinned',FW,sha,'; README, changelog, simulator, installer updated')
