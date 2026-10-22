#!/usr/bin/env python3
"""Tests real V2 FLASH -> BD4S cache -> prefetch -> native FM1 mono PCM."""
from pathlib import Path
import math,subprocess,tempfile,sys,struct
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/storage_v2'))
sys.path.insert(0,str(ROOT/'tools/dpcm4'))
from storage_v2_sim import Storage,Nor
import beetle_dpcm4 as bd4
SRC=ROOT/'firmware/fm1-native'
with tempfile.TemporaryDirectory(prefix='v1422-live-') as td:
 t=Path(td)
 pcm=[int(12500*math.sin((2*math.pi*178*i)/22050)) for i in range(23041)]
 raw=bd4.encode(pcm)
 # Must straddle multiple physical erase records and independent codec blocks.
 assert len(raw)>4068
 flash=Nor(authorized=True)
 volume=Storage(flash,None)
 volume.replace_sample(0,raw)
 (t/'volume.bin').write_bytes(flash.bytes)
 cc=['cc','-std=c11','-O2','-fsanitize=undefined','-fno-sanitize-recover=all','-I',str(SRC),
     str(ROOT/'tests/dpcm4/test_v1422_fm1_live.c'),str(SRC/'beetle_bd4_live.c'),
     str(SRC/'beetle_bd4_prefetch.c'),str(SRC/'beetle_storage_v2_reader.c'),
     str(SRC/'beetle_project_pack_v2.c'),'-o',str(t/'live')]
 subprocess.run(cc,check=True)
 subprocess.run([str(t/'live'),str(t/'volume.bin'),str(t/'out.raw')],check=True)
 six=['cc','-std=c11','-O2','-fsanitize=undefined','-fno-sanitize-recover=all','-I',str(SRC),
      str(ROOT/'tests/dpcm4/test_v1422_fm1_six_voice.c'),str(SRC/'beetle_bd4_live.c'),
      str(SRC/'beetle_bd4_prefetch.c'),str(SRC/'beetle_storage_v2_reader.c'),
      str(SRC/'beetle_project_pack_v2.c'),'-o',str(t/'six')]
 subprocess.run(six,check=True)
 subprocess.run([str(t/'six'),str(t/'volume.bin')],check=True)
 actual=t.joinpath('out.raw').read_bytes()
 pcm_ref=bd4.decode(raw)
 refs=[]
 for pos in range(0,40000):
  frame=pos>>1
  if frame>=len(pcm_ref):break
  a=pcm_ref[frame];b=pcm_ref[frame+1] if frame+1<len(pcm_ref) else a
  refs.append(((abs(a+b)//2)*(1 if a+b>=0 else -1)) if pos&1 else a)
 actual_pcm=list(struct.unpack('<'+'h'*(len(actual)//2),actual))

 pairs=[(k,a,b) for k,(a,b) in enumerate(zip(actual_pcm,refs)) if a!=b]
 print('mismatches:',len(pairs),'first:',pairs[:12])
 assert not pairs,f'PCM mismatch: {len(actual_pcm)} frames'
 print(f'PASS actual Storage V2 to FM1 BD4S PCM: {len(actual_pcm)} samples, exact reference')
