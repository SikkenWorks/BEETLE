#!/usr/bin/env python3
"""Native compiled C audio parity to V1.6.10: 48 diverse scenes across all 12 types.
Supplied user projects are *not* included in this release or test fixtures.
"""
from pathlib import Path
import subprocess,tempfile,hashlib
root=Path(__file__).resolve().parents[2]
fw=root/'firmware/fm1-native'
probe=root/'tests/track/probe_v177_12type_pcm.c'
reference=root/'tests/track/V1610-ALL-TYPES-PCM-FINGERPRINTS.csv'
with tempfile.TemporaryDirectory() as tmp:
    exe=Path(tmp)/'check'
    subprocess.run(['cc','-std=c11','-O2','-I'+str(fw),str(probe),str(fw/'track_chip.c'),str(fw/'track_mixer.c'),'-o',str(exe)],check=True)
    actual=subprocess.check_output([str(exe)],text=True)
assert actual==reference.read_text(),'REGRESSION: rendered audio differs from V1.6.10 for one or more oscillator types'
assert len(actual.splitlines())==48
# Check global audio safety remains retained, separate from synth PCM (verified at runtime in real FM1 only).
assert (fw/'beetle_audio_safety.h').is_file()
assert 'beetle_master_safe_gain' in (fw/'audio_target.c').read_text()
assert 'native_song_stop_pending' in (fw/'audio_target.c').read_text()
assert hashlib.sha256((fw/'track_chip.c').read_bytes()).hexdigest()=='a3bab76bfce2cf99e690f97fe40fdfc1a1d30d7078ae283b6d377f7168160cef'
print('PASS 48/48 PCM scenes across all 12 oscillators match V1.6.10; safety entrypoints retained')
