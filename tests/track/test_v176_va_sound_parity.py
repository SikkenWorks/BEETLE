#!/usr/bin/env python3
"""V1.7.6 VA patches must render bit-identical to V1.6.10 host reference.
Do not weaken this by changing the expected fixture when editing synth gain.
"""
from pathlib import Path
import hashlib,subprocess,tempfile
r=Path(__file__).resolve().parents[2]
fw=r/'firmware/fm1-native'
source=r/'tests/track/probe_v176_va_pcm.c'
expected=(r/'tests/track/V1610-VA-PCM-FINGERPRINTS.csv').read_text()
with tempfile.TemporaryDirectory() as d:
 exe=Path(d)/'va-native-regression'
 subprocess.run(['cc','-std=c11','-O2','-I'+str(fw),str(source),str(fw/'track_chip.c'),str(fw/'track_mixer.c'),'-o',str(exe)],check=True)
 actual=subprocess.check_output([str(exe)],text=True)
 assert actual==expected,'VA OSC / OSC2 altered relative to V1.6.10 PCM fixtures'
 assert len(actual.splitlines())==40
 assert max(int(s.rsplit(',',1)[1]) for s in actual.splitlines())<30000
 # No hidden changes to oscillators, output/master safety or UI latency paths.
 original_hashes={
  'track_chip.c':'a3bab76bfce2cf99e690f97fe40fdfc1a1d30d7078ae283b6d377f7168160cef',
  'track_drum.c':'81e8f077e4f8f4730ecfd92bedc4e5a2e94c425d6f5be0f301dc91b172811a9d',
  'track_mixer.c':'6dbea1ecf3d385c5c3e73666aa69c6a89a717ab3c0f7a1f137726ba90aa9ba68',
  'track_song.c':'95c6bab97e6e6ffe429eb4e6c4ec026f4915fe0f4c8de533034224c0751aedfe',
  'lcd.c':'bd2e7421d1e309ba3addd3e7071ef41694fc2083ccf23467cf1b6b99a19bb50b'
 }
 for name,digest in original_hashes.items():
  assert hashlib.sha256((fw/name).read_bytes()).hexdigest()==digest,name
 g=(fw/'beetle_wave_balance.h').read_text()
 assert '105u,245u,256u,345u,410u,403u' in g
 assert '631u,347u,525u,214u,181u,244u' in g
 assert '323u,314u,185u,213u,367u' in g
 assert 'BEETLE_MASTER_STALE_MS' in (fw/'beetle_audio_safety.h').read_text()
 print('PASS V1.7.7: 40 VA PCM reference scenes EXACTLY match V1.6.10; peaks <30000; safety and DSP invariants preserved')
