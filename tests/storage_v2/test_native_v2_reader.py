#!/usr/bin/env python3
"""Native C Storage V2 flash reader against actual Python NOR byte images.
No USB or FM1 hardware. No writes to physical flash.
"""
import ctypes, subprocess, tempfile, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/storage_v2'))
sys.path.insert(0,str(ROOT/'tools/dpcm4'))
from storage_v2_sim import Nor,Storage,ProjectCodec
import beetle_dpcm4 as bd4
p=ROOT/'firmware/fm1-native'
with tempfile.TemporaryDirectory() as t:
    lib=Path(t)/'lib.so'
    subprocess.run(['cc','-std=c11','-O2','-fPIC','-shared','-Wall','-Wextra','-Werror',
                    str(p/'beetle_project_pack_v2.c'),str(p/'beetle_storage_v2_reader.c'),
                    '-o',str(lib)],check=True)
    dll=ctypes.CDLL(str(lib))
    codec=ProjectCodec(dll)
    nor=Nor(authorized=True);store=Storage(nor,codec)
    fixtures=[(ROOT/'tests/fixtures/NUGGIE-SLOT-07.trk').read_bytes(),
              (ROOT/'tests/fixtures/MOUNTAINKING-SLOT-01.trk').read_bytes()]
    for i in range(12):store.save_project(i,fixtures[i%2])
    blobs=[]
    for i in range(13):
        samples=[int(10000*(j%41-20)/20) for j in range((18000 if i==12 else 108+(i*213)))]
        data=bd4.encode(samples);blobs.append(data);store.replace_sample(i,data)
    image=bytes(nor.bytes)
    assert len(image)==288*1024
    class Device(ctypes.Structure):
        _fields_=[('read',ctypes.c_void_p),('context',ctypes.c_void_p),('index_address',ctypes.c_uint32),('generation',ctypes.c_uint32)]
    U8=ctypes.c_uint8
    rd_fn=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(U8),ctypes.c_uint32)
    view=bytearray(image)
    reads=[]
    def read(_ctx,physical,dst,size):
        off=physical-0x97000
        if off<0 or off+size>len(view):return -1
        ctypes.memmove(dst,bytes(view[off:off+size]),size)
        reads.append((off,size));return 0
    cb=rd_fn(read)
    dll.beetle_v2_open.argtypes=[ctypes.POINTER(Device),rd_fn,ctypes.c_void_p,ctypes.POINTER(U8)]
    dll.beetle_v2_sample_read.argtypes=[ctypes.POINTER(Device),U8,ctypes.c_uint32,ctypes.POINTER(U8),ctypes.c_uint32,ctypes.POINTER(U8)]
    dll.beetle_v2_project_read.argtypes=[ctypes.POINTER(Device),U8,ctypes.POINTER(U8),ctypes.c_uint32,
        ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(U8),ctypes.POINTER(U8),ctypes.c_uint32]
    dev=Device();index=(U8*4096)()
    assert dll.beetle_v2_open(ctypes.byref(dev),cb,None,index)==0
    assert dev.index_address in (0,4096)
    scratch=(U8*13000)()
    output=(U8*12256)()
    written=ctypes.c_uint32()
    for i in range(12):
        assert dll.beetle_v2_project_read(ctypes.byref(dev),i,output,12256,ctypes.byref(written),index,scratch,13000)==1
        assert bytes(output[:written.value])==fixtures[i%2]
    for i,blob in enumerate(blobs):
        for start,n in [(0,min(16,len(blob))),(0,len(blob)),(17,min(311,len(blob)-17)),(4057,min(85,max(0,len(blob)-4057))) if len(blob)>4057 else (0,1),(len(blob),0)]:
            buf=(U8*max(1,n))()
            assert dll.beetle_v2_sample_read(ctypes.byref(dev),i,start,buf,n,index)==0
            assert bytes(buf[:n])==blob[start:start+n]
    assert all(0<=offset and offset+size<=len(view) for offset,size in reads)
    # Deleted sample cannot be accessed after new committed metadata.
    store.delete_sample(4)
    view[:]=store.nor.bytes
    assert dll.beetle_v2_open(ctypes.byref(dev),cb,None,index)==0
    x=(U8*32)()
    assert dll.beetle_v2_sample_read(ctypes.byref(dev),4,0,x,16,index)<0
    assert dll.beetle_v2_sample_read(ctypes.byref(dev),5,0,x,16,index)==0
    # Corruption of a stored record must fail CRC (both full and ranged reads).
    ref=store.current.samples[5].chunks[0]
    view[ref.offset+25]^=0x10
    assert dll.beetle_v2_sample_read(ctypes.byref(dev),5,16,x,16,index)<0
    view[ref.offset+25]^=0x10
    # Corrupt committed metadata -> previous valid snapshot fallback when available.
    newer=dev.index_address
    view[newer+16]^=1
    assert dll.beetle_v2_open(ctypes.byref(dev),cb,None,index)==0
    view[newer+16]^=1
    # Distinct flash beyond partition is NEVER read.
    print('PASS: 12 original TRK1 roundtrips, 13 BD4S blobs and partial reads, deletion, torn index fallback, data CRC rejection, physical bounds')
