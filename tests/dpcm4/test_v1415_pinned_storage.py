#!/usr/bin/env python3
"""Real C prefetch + immutable V2 snapshot leases; offline only, no device writes."""
import ctypes
import math
import subprocess
import tempfile
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/storage_v2'))
sys.path.insert(0,str(root/'tools/dpcm4'))
from storage_v2_sim import Storage,Nor,ProjectCodec,CorruptData,NotEnoughSafeSpace,SECTOR,SECTORS
import beetle_dpcm4 as dpcm

with tempfile.TemporaryDirectory() as tmp:
    tmp=Path(tmp);source=root/'firmware/fm1-native'
    common=['cc','-std=c11','-O2','-fPIC','-shared','-Wall','-Wextra','-Werror']
    subprocess.run(common+[str(source/'beetle_project_pack_v2.c'),'-o',str(tmp/'p.so')],check=True)
    subprocess.run(common+['-I',str(source),str(source/'beetle_bd4_prefetch.c'),
                   str(root/'tests/dpcm4/test_v1415_pinned_bridge.c'),'-o',str(tmp/'audio.so')],check=True)
    lib=ctypes.CDLL(str(tmp/'audio.so'))
    db=ctypes.POINTER(ctypes.c_uint8)
    cb_type=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint8,ctypes.c_uint8,
                            ctypes.c_uint32,ctypes.c_uint32,db,ctypes.c_uint16)
    old_type=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint8,
                             ctypes.c_uint32,db,ctypes.c_uint16)
    lib.pin_prepare.argtypes=[ctypes.c_uint,ctypes.c_uint8,ctypes.c_uint32,ctypes.c_uint32]
    lib.pin_prefetch.argtypes=[cb_type,ctypes.c_void_p,ctypes.c_uint]
    lib.pin_unpinned_poll.argtypes=[old_type,ctypes.c_void_p,ctypes.c_uint]
    lib.pin_render.argtypes=[ctypes.c_uint,ctypes.POINTER(ctypes.c_int16),ctypes.c_uint32]
    lib.pin_start.argtypes=[ctypes.c_uint]
    lib.pin_underruns.argtypes=[ctypes.c_uint]
    lib.pin_generation.argtypes=[ctypes.c_uint]
    nor=Nor(authorized=True);codec=ProjectCodec(ctypes.CDLL(str(tmp/'p.so')))
    store=Storage(nor,codec)
    nuggie=(root/'tests/fixtures/NUGGIE-SLOT-07.trk').read_bytes()
    for i in range(12):store.save_project(i,nuggie)
    def make(freq,length):
        pcm=[int(12000*math.sin(2*math.pi*freq*i/22050)) for i in range(length)]
        blob=dpcm.encode(pcm)
        return blob,dpcm.decode(blob)
    old_blob,old_pcm=make(133,3400)
    new_blob,new_pcm=make(347,3800)
    slot=5
    store.replace_sample(slot,old_blob)
    old=store.acquire_sample(slot)
    lib.pin_reset()
    assert lib.pin_prepare(0,slot,len(old_blob),old.entry.generation)==0
    assert lib.pin_prepare(2,slot,len(old_blob),0)==-1
    owned={0:old}
    in_render=False
    reads=[]
    bad=[]
    def callback(_,voice,slot_id,generation,offset,dest,size):
        if in_render:bad.append('I/O in render');return 0
        lease=owned.get(voice)
        if not lease or slot_id!=lease.slot or generation!=lease.entry.generation:
            bad.append('generation mismatch');return 0
        try: chunk=store.read_sample_lease(lease,offset,size)
        except CorruptData:bad.append('expired/corrupt lease');return 0
        ctypes.memmove(dest,chunk,size)
        reads.append((voice,generation,offset,size))
        return 1
    reader=cb_type(callback)
    unsafe=old_type(lambda *a:1)
    assert lib.pin_unpinned_poll(unsafe,None,12)==-2
    assert lib.pin_prefetch(reader,None,2)==2
    assert lib.pin_start(0)==1
    # Old sample must remain playable after a complete replacement + metadata retirement.
    store.replace_sample(slot,new_blob)
    newer=store.acquire_sample(slot)
    assert newer.entry.generation==old.entry.generation+1
    owned[1]=newer
    assert lib.pin_prepare(1,slot,len(new_blob),newer.entry.generation)==0
    assert lib.pin_prefetch(reader,None,2)>=2
    assert lib.pin_start(1)==1
    old_start=old.entry.chunks[0].offset
    pinned_sector=old_start//SECTOR
    assert pinned_sector in store._live_sectors()
    store.checkpoint() # old index may be retired, but active voice keeps old record
    assert store.read_sample_lease(old,0,len(old_blob))==old_blob
    assert store.read_sample_lease(newer,0,len(new_blob))==new_blob
    assert pinned_sector in store._live_sectors()
    output=(ctypes.c_int16*128)()
    for pos in range(0,max(len(old_pcm),len(new_pcm))*2+128,128):
        assert lib.pin_prefetch(reader,None,8)>=0
        for voice,pcm in ((0,old_pcm),(1,new_pcm)):
            in_render=True
            try:lib.pin_render(voice,output,128)
            finally:in_render=False
            for k in range(128):
                t=pos+k;frame=t//2
                if frame>=len(pcm):wanted=0
                elif t&1:wanted=int((pcm[frame]+pcm[min(frame+1,len(pcm)-1)])/2)
                else:wanted=pcm[frame]
                assert output[k]==wanted,(voice,t,output[k],wanted)
    assert lib.pin_underruns(0)==lib.pin_underruns(1)==0
    assert not bad,bad
    assert len(reads)>12
    store.delete_sample(slot)
    store.checkpoint()
    assert store.load_sample(slot) is None
    assert store.read_sample_lease(newer,0,len(new_blob))==new_blob
    assert store.read_sample_lease(old,0,len(old_blob))==old_blob
    # Stop polling this generation BEFORE retiring its storage lease.
    lib.pin_disarm(0)
    old_reads=len(reads)
    assert lib.pin_prefetch(reader,None,2)>=0
    assert not any(v==0 for v,_,_,_ in reads[old_reads:])
    store.release_sample(old)
    try:store.read_sample_lease(old,0,1)
    except CorruptData:pass
    else:raise AssertionError('expired lease still readable')
    lib.pin_disarm(1)
    store.release_sample(newer)
    try:store.release_sample(newer)
    except CorruptData:pass
    else:raise AssertionError('double-release permitted')
    store.checkpoint()
    assert store.stats()['pinned_generations']==0
    assert all(store.load_project(i)==nuggie for i in range(12))
    print('PASS pinned generations: old and new same slot concurrently, PCM exact, no ISR reads')
    print('PASS replace/delete/checkpoint during playback; samples pinned until release')
    print('PASS stale lease rejected; 12 projects unchanged; zero underruns')
    # Verify reserve measured as whole erased sectors, not a few spare pages.
    guard=Nor(authorized=True);gs=Storage(guard,codec)
    for sect in range(2,SECTORS-5):
        guard.bytes[sect*SECTOR:(sect+1)*SECTOR]=b'\x00'*SECTOR
    assert gs.stats()['erased_sectors']==5
    before=bytes(guard.bytes)
    try:gs.replace_sample(0,b'Q'*6500) # requires two sectors, would leave only three
    except NotEnoughSafeSpace:pass
    else:raise AssertionError('4-sector safety margin violated')
    assert bytes(guard.bytes)==before
    print('PASS four WHOLE erased-sector reserve; insufficient space is atomic refusal')
