#!/usr/bin/env python3
"""Offline-only realistic BD4S data vs. sector-granular V2 storage.
Counts actual encoded blocks, independent U01..U13 objects, real project fixtures.
"""
import ctypes
import importlib.util
import math
import struct
import sys
import tempfile
import zlib
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/storage_v2'))
from storage_v2_sim import Nor,Storage,ProjectCodec,NotEnoughSafeSpace,UnsafeLayout
mod=root/'tools/dpcm4/beetle_dpcm4.py'
spec=importlib.util.spec_from_file_location('codec',mod)
dpcm=importlib.util.module_from_spec(spec);spec.loader.exec_module(dpcm)
with tempfile.TemporaryDirectory() as td:
    c=root/'firmware/fm1-native/beetle_project_pack_v2.c';so=Path(td)/'packer.so'
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fPIC','-shared',str(c),'-o',str(so)],check=True)
    pc=ProjectCodec(ctypes.CDLL(str(so)))
    fixtures=root/'tests/fixtures'
    mountain=(fixtures/'MOUNTAINKING-SLOT-01.trk').read_bytes()
    nuggie=(fixtures/'NUGGIE-SLOT-07.trk').read_bytes()
    # Single 4096-frame sine burst, repeat whole independently coded blocks.
    base=[int((8000+4000*math.sin(2*math.pi*i/2048))*math.sin(2*math.pi*220*i/22050)) for i in range(4096)]
    baseline=dpcm.encode(base)[16:]
    assert len(baseline)>0
    def bank(nblocks):
        payload=baseline*nblocks
        frames=4096*nblocks
        result=dpcm.HEADER.pack(b'BD4S',1,1,22050,frames,zlib.crc32(payload)&0xffffffff)+payload
        # Positive decoder validation for every assembled independent sample file
        assert len(dpcm.decode(result))==frames
        return result
    for sec in (10,20):
        # Split approximately evenly across the 13 global U slots, preserving all 12 projects.
        blocks=round(sec*22050/4096)
        pieces=[blocks//13+(1 if i<blocks%13 else 0) for i in range(13)]
        pieces=[b for b in pieces if b]
        dev=Nor(authorized=True);storage=Storage(dev,pc)
        for i in range(12):storage.save_project(i,mountain if i==0 else nuggie)
        filled=0;success=True;failure_reason=''
        for i,b in enumerate(pieces):
            try:storage.replace_sample(i,bank(b))
            except NotEnoughSafeSpace as e:
                success=False;failure_reason=str(e);break
            filled+=1
        assert all(storage.load_project(i)==(mountain if i==0 else nuggie) for i in range(12))
        assert storage.stats()['erased_sectors']>=4
        print(f'{sec}s requested: actual duration={blocks*4096/22050:.2f}s; U slots={len(pieces)}; stored={filled}; success={success}; remaining={storage.stats()["erased_sectors"]} erased sectors; {failure_reason}')
        if not success:print('Capacity refusal was non-destructive; 12 projects verified unchanged')
    print('Real BD4S payload / 12-project offline sector-capacity integration PASS')
