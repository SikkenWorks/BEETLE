#!/usr/bin/env python3
"""Read compressed BD4S through real Storage V2 chunk/CRC logic into native C.

Models sector usage, mutable transactions, restart, and streaming; NEVER writes FM1.
"""
from pathlib import Path
import ctypes, math, subprocess, tempfile, sys
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/storage_v2'))
sys.path.insert(0,str(root/'tools/dpcm4'))
from storage_v2_sim import Storage, Nor, ProjectCodec, CorruptData
import beetle_dpcm4 as dpcm

with tempfile.TemporaryDirectory() as td:
    td=Path(td)
    p=root/'firmware/fm1-native'
    subprocess.run(['cc','-std=c11','-O2','-fPIC','-shared','-Wall','-Wextra','-Werror',
        str(p/'beetle_project_pack_v2.c'),'-o',str(td/'pack.so')],check=True)
    subprocess.run(['cc','-std=c11','-O2','-fPIC','-shared','-Wall','-Wextra','-Werror',
        '-I',str(p),str(p/'beetle_bd4_prefetch.c'),
        str(root/'tests/dpcm4/test_v1414_storage_stream.c'),'-o',str(td/'stream.so')],check=True)
    c=ctypes.CDLL(str(td/'stream.so'))
    u8p=ctypes.POINTER(ctypes.c_uint8)
    i16p=ctypes.POINTER(ctypes.c_int16)
    cb_type=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint8,ctypes.c_uint32,u8p,ctypes.c_uint16)
    c.beetle_test_prepare.argtypes=[ctypes.c_uint,ctypes.c_uint8,ctypes.c_uint32]
    c.beetle_test_poll.argtypes=[cb_type,ctypes.c_void_p,ctypes.c_uint]
    c.beetle_test_start.argtypes=[ctypes.c_uint]
    c.beetle_test_render.argtypes=[ctypes.c_uint,i16p,ctypes.c_uint32]
    c.beetle_test_underruns.argtypes=[ctypes.c_uint]
    nor=Nor(authorized=True)
    storage=Storage(nor,ProjectCodec(ctypes.CDLL(str(td/'pack.so'))))
    fixture=root/'tests/fixtures/NUGGIE-SLOT-07.trk'
    project=fixture.read_bytes()
    for j in range(12):storage.save_project(j,project)
    encoded=[];decoded=[];reads=[]
    for slot in range(13):
        frames=384+slot*113
        pcm=[int((8000+slot*97)*math.sin(2*math.pi*(120+slot*11)*i/22050)) for i in range(frames)]
        blob=dpcm.encode(pcm);want=dpcm.decode(blob)
        encoded.append(blob);decoded.append(want)
        storage.replace_sample(slot,blob)
    # This bank survived a simulated reboot, with 12 independent saved projects.
    storage=Storage(Nor(bytes(nor.bytes),authorized=True),storage.codec)
    assert all(storage.load_project(j)==project for j in range(12))
    assert all(storage.load_sample(s)==encoded[s] for s in range(13))
    callback_in_render=False
    errors=[]
    def read(_ctx, slot, off, dst, length):
        if callback_in_render:
            errors.append('reader called inside audio render')
            return 0
        try:
            block=storage.read_sample_range(slot,off,length)
        except (CorruptData,ValueError) as e:
            errors.append(f'reader rejected: {e}')
            return 0
        ctypes.memmove(dst,block,length)
        reads.append((slot,off,length))
        return 1
    reader=cb_type(read)
    def play(slots):
        nonlocal_keys=None
        global callback_in_render
        c.beetle_test_reset()
        for voice,slot in enumerate(slots):
            assert c.beetle_test_prepare(voice,slot,len(encoded[slot]))==0
            assert c.beetle_test_start(voice)==0 # always require prefetch
        assert c.beetle_test_poll(reader,None,2*len(slots))==2*len(slots)
        for voice,_ in enumerate(slots):assert c.beetle_test_start(voice)==1
        length=max(len(decoded[s])*2 for s in slots)
        out=(ctypes.c_int16*128)()
        for pos in range(0,length,128):
            assert c.beetle_test_poll(reader,None,len(slots))>=0
            for voice,slot in enumerate(slots):
                callback_in_render=True
                try:c.beetle_test_render(voice,out,128)
                finally:callback_in_render=False
                data=decoded[slot]
                for k in range(128):
                    index=(pos+k)//2
                    want=0
                    if index<len(data):
                        a=data[index];b=data[min(index+1,len(data)-1)]
                        want=int((a+b)/2) if ((pos+k)&1) else a
                    assert out[k]==want, (slot,pos+k,out[k],want)
            assert all(c.beetle_test_underruns(v)==0 for v in range(len(slots)))
    play([0,1,2,3,4,5]);play([6,7,8,9,10,11]);play([12])
    # Transactional replacement: prefetch sees the committed generation only.
    slot=4
    new_pcm=[round(10000*math.sin(i/21)) for i in range(640)]
    changed=dpcm.encode(new_pcm)
    storage.replace_sample(slot,changed)
    storage.checkpoint()
    storage=Storage(Nor(bytes(storage.nor.bytes),authorized=True),storage.codec)
    encoded[slot]=changed;decoded[slot]=dpcm.decode(changed)
    play([4])
    # Modified/invalid data must never be passed to the decoder unchecked.
    entry=storage.current.samples[4];ref=entry.chunks[0]
    corrupt_offset=ref.offset+24+10
    previous=storage.nor.bytes[corrupt_offset]
    storage.nor.bytes[corrupt_offset]^=1
    try:
        storage.read_sample_range(4,10,10)
        raise AssertionError('corrupt sector accepted')
    except CorruptData:pass
    finally:storage.nor.bytes[corrupt_offset]=previous
    storage.delete_sample(4)
    assert storage.load_sample(4) is None
    assert all(storage.load_project(j)==project for j in range(12))
    assert len(reads)>100 and not errors,errors
    print('PASS: 13 slots, six simultaneous voices, exact 44.1k PCM, no ISR storage access')
    print('PASS: 12 projects and 13 samples after restart, replace, reclaim, CRC rejection, delete')
    print('PASS: immutable sector-backed reads and zero underruns with scheduled prefetch')
