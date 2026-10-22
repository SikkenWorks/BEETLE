#!/usr/bin/env python3
"""Original BD4S: deterministic Python/C, CRC/truncation, capacity, quality."""
import ctypes
import importlib.util
import math
import random
import struct
import subprocess
import tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
MOD=ROOT/'tools/dpcm4/beetle_dpcm4.py'
spec=importlib.util.spec_from_file_location('beetle_dpcm4',MOD)
bd4=importlib.util.module_from_spec(spec);spec.loader.exec_module(bd4)
src=ROOT/'firmware/fm1-native'

with tempfile.TemporaryDirectory() as temp:
    so=Path(temp)/'libbeetle_bd4.so'
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fPIC','-shared','-I',str(src),str(src/'beetle_dpcm4.c'),'-o',str(so)],check=True)
    lib=ctypes.CDLL(str(so))
    lib.beetle_bd4_decode_block.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32,ctypes.POINTER(ctypes.c_int16),ctypes.c_uint32]
    lib.beetle_bd4_decode_block.restype=ctypes.c_int
    def native(data):
        raw=(ctypes.c_uint8*len(data)).from_buffer_copy(data)
        out=(ctypes.c_int16*256)()
        got=lib.beetle_bd4_decode_block(raw,len(data),out,256)
        return list(out[:got]) if got>=0 else got
    random.seed(148)
    sources={
        'silence':[0]*2048,
        'constant':[2600]*2048,
        'sine':[int(15000*math.sin(i*2*math.pi*440/22050)) for i in range(4097)],
        'high frequency':[int(11000*math.sin(i*2*math.pi*7000/22050)) for i in range(4096)],
        'drum decay':[int(22000*math.exp(-i/1500)*math.sin(i*2*math.pi*(250-120*min(i/4096,1))/22050)) for i in range(4096)],
        'random':[random.randint(-29000,29000) for _ in range(4096)],
        'saturation':[32767,-32768]*1024,
        'one frame':[12345],
    }
    for name,pcm in sources.items():
        packed=bd4.encode(pcm)
        decoded=bd4.decode(packed)
        assert len(decoded)==len(pcm),(name,'frame mismatch')
        assert all(-32768<=x<=32767 for x in decoded)
        payload=packed[16:];offset=0;result=[]
        while offset<len(payload):
            count=struct.unpack_from('<H',payload,offset+4)[0]
            size=6+count//2
            assert native(payload[offset:offset+size])==decoded[len(result):len(result)+count],name
            assert native(payload[offset:offset+size-1])<0
            result.extend(decoded[len(result):len(result)+count]);offset+=size
        assert offset==len(payload)
        err=sum((a-b)**2 for a,b in zip(pcm,decoded))
        power=sum(v*v for v in pcm)
        snr=10*math.log10(power/err) if err and power else float('inf')
        if name in ('sine','drum decay'):assert snr>15,(name,snr)
        bad=bytearray(packed);bad[-1]^=1
        try:bd4.decode(bytes(bad))
        except bd4.CodecError:pass
        else:raise AssertionError('CRC not detected')
        print(f'{name:>15}: {len(pcm):6d} frames, {len(packed):6d} encoded bytes, SNR={snr:5.1f} dB, native MATCH')
    assert native(b'\0'*6)<0
    assert len(bd4.encode([0]*22050))<12000
    assert len(bd4.encode([0]*441000))<240000
    print('BD4S Python/C block interoperability + integrity + 10s/20s sizing: PASS')
