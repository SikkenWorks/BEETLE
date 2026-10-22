#!/usr/bin/env python3
"""BEETLE V1.4.19: independent project/sample sector domains, real C audit.

Runs only on an in-memory 288KiB NOR emulator. No FM1 device access.
"""
import ctypes
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/storage_v2'))
from storage_v2_sim import Nor,Storage,ProjectCodec,NotEnoughSafeSpace,SECTOR
P=ROOT/'firmware/fm1-native'

with tempfile.TemporaryDirectory(prefix='beetle-coexist-') as t:
    so=Path(t)/'storage.so'
    subprocess.run(['cc','-std=c11','-O2','-fPIC','-shared','-Wall','-Wextra','-Werror',
                    str(P/'beetle_storage_v2_reader.c'),str(P/'beetle_project_pack_v2.c'),
                    '-o',str(so)],check=True)
    dll=ctypes.CDLL(str(so));codec=ProjectCodec(dll)
    nuggie=(ROOT/'tests/fixtures/NUGGIE-SLOT-07.trk').read_bytes()
    mountain=(ROOT/'tests/fixtures/MOUNTAINKING-SLOT-01.trk').read_bytes()
    class Dev(ctypes.Structure):
        _fields_=[('read',ctypes.c_void_p),('context',ctypes.c_void_p),
                  ('index_address',ctypes.c_uint32),('generation',ctypes.c_uint32)]
    class Domains(ctypes.Structure):
        _fields_=[('project_sectors',ctypes.c_uint16),('sample_sectors',ctypes.c_uint16),
                  ('assigned_projects',ctypes.c_uint16),('assigned_samples',ctypes.c_uint16)]
    rd_type=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,
                            ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32)
    u8=ctypes.c_uint8
    dll.beetle_v2_open.argtypes=[ctypes.POINTER(Dev),rd_type,ctypes.c_void_p,ctypes.POINTER(u8)]
    dll.beetle_v2_audit_domains.argtypes=[ctypes.POINTER(Dev),ctypes.POINTER(Domains),ctypes.POINTER(u8)]
    backing=bytearray(288*1024)
    def read(_ctx,address,out,n):
        at=address-0x97000
        if at<0 or at+n>len(backing):return -1
        ctypes.memmove(out,bytes(backing[at:at+n]),n)
        return 0
    callback=rd_type(read)
    dev=Dev();scratch=(u8*4096)()
    def native_audit(store):
        backing[:]=store.nor.bytes
        assert dll.beetle_v2_open(ctypes.byref(dev),callback,None,scratch)==0
        result=Domains()
        assert dll.beetle_v2_audit_domains(ctypes.byref(dev),ctypes.byref(result),scratch)==0
        stats=store.domain_stats()
        assert stats['shared_legacy_sectors']==0
        current_projects=set()
        current_samples=set()
        for pair in store.current.projects:
            for item in pair:
                if item:current_projects.update(store._sectors_of_ref(item.ref))
        for entry in store.current.samples:
            if entry:
                for ref in entry.chunks:current_samples.update(store._sectors_of_ref(ref))
        assert result.project_sectors==len(current_projects)
        assert result.sample_sectors==len(current_samples)
        assert result.project_sectors<=stats['project_sectors']
        assert result.sample_sectors<=stats['sample_sectors']
        return result

    nor=Nor(authorized=True);s=Storage(nor,codec)
    for i in range(12):s.save_project(i,mountain if i==0 else nuggie)
    # ~20 seconds of actual DPCM4-sized payloads split over 13 shared U slots.
    # This is a storage-capacity test; the separate BD4S test checks codec/PCM.
    sizes=[17494]*12+[17470]
    # 227398 bytes total, close to DPCM4's 22.05kHz 20-second budget.
    blobs=[bytes(((j*17+i*31)+(j//23))&255 for j in range(size)) for i,size in enumerate(sizes)]
    for i,blob in enumerate(blobs):s.replace_sample(i,blob)
    d=native_audit(s)
    assert d.assigned_projects==12 and d.assigned_samples==13
    assert s.stats()['erased_sectors']>=4
    assert all(s.load_project(i)==(mountain if i==0 else nuggie) for i in range(12))
    assert all(s.load_sample(i)==blobs[i] for i in range(13))
    print('PASS: 12 project slots + 13 sample slots at ~20 seconds; zero mixed sectors',s.domain_stats())

    # Project edits must NEVER overwrite one of the sample's reserved sectors.
    s.save_project(1,mountain)
    assert s.load_sample(12)==blobs[12]
    native_audit(s)
    s.delete_project(1)
    assert s.load_project(1) is None
    assert s.load_sample(12)==blobs[12]
    s.checkpoint()
    native_audit(s)
    print('PASS: project overwrite/delete/checkpoint does not disturb sample area')

    # Refresh U12 without affecting ANY project. A live lease pins a deleted
    # generation until audio work releases that lease explicitly.
    lease=s.acquire_sample(12)
    oldpart=s.read_sample_lease(lease,17,128)
    try:
        s.replace_sample(12,b'q'*300)
    except NotEnoughSafeSpace:
        # A bounded volume must reject growth without touching existing data.
        # With near-20s already occupied, freeing U12 in a checkpoint may be
        # necessary before it can be replaced transactionally.
        s.release_sample(lease)
        s.delete_sample(12)
        s.checkpoint()
        lease=None
        s.replace_sample(12,b'q'*300)
    else:
        assert s.read_sample_lease(lease,17,128)==oldpart
        s.release_sample(lease)
    assert s.load_sample(12)==b'q'*300
    assert s.load_project(0)==mountain
    native_audit(s)
    print('PASS: immutable sample generation; slot replacement independent of project storage')

    restored=Storage(Nor(bytes(s.nor.bytes),authorized=True),codec)
    native_audit(restored)
    assert restored.load_project(0)==mountain
    assert restored.load_project(1) is None
    assert restored.load_sample(12)==b'q'*300
    print('PASS: cold restart, independent namespaced indices and CRC verification')

    # When project slots are incompressible, the sample bank may NOT steal
    # required space: detect capacity before the first NOR mutation.
    import random
    hard=Storage(Nor(authorized=True),codec)
    rng=random.Random(1914)
    originals=[rng.randbytes(12256) for _ in range(12)]
    for i,proj in enumerate(originals):hard.save_project(i,proj)
    before=bytes(hard.nor.bytes)
    try:hard.replace_sample(0,b'A'*230000)
    except NotEnoughSafeSpace:pass
    else:raise AssertionError('incompressible projects should prevent allocation')
    assert bytes(hard.nor.bytes)==before
    assert all(hard.load_project(i)==originals[i] for i in range(12))
    print('PASS: full-storage transaction rejected BEFORE writing, existing projects intact')

    # A deliberately forged CRC-correct index must not be able to assign
    # sample chunks into a live project sector (native independent audit).
    import struct,zlib
    bad=bytearray(restored.nor.bytes)
    index_base=restored.current.sector*SECTOR
    idx=bytearray(bad[index_base:index_base+SECTOR])
    pos=20+12*32
    # Locate assigned U01 and redirect it to a PROJECT sector on a PAGE
    # boundary, then recompute metadata CRC so the index remains syntactically
    # valid; sector ownership audit must still reject it.
    assert struct.unpack_from('<I',idx,pos)[0]>0
    project_sector=restored.current.projects[0][0].ref.offset//SECTOR
    struct.pack_into('<I',idx,pos+14,project_sector*SECTOR)
    payload_size=struct.unpack_from('<I',idx,12)[0]
    checksum=zlib.crc32(idx[:16]+idx[20:20+payload_size])&0xffffffff
    struct.pack_into('<I',idx,16,checksum)
    bad[index_base:index_base+SECTOR]=idx
    backing[:]=bad
    assert dll.beetle_v2_open(ctypes.byref(dev),callback,None,scratch)==0
    assert dll.beetle_v2_audit_domains(ctypes.byref(dev),ctypes.byref(Domains()),scratch)==-2
    print('PASS: native rejects CRC-correct forged mixed-sector layout')
    from report_coexistence import report
    image_file=Path(t)/'coexistent-v2.bin'
    image_file.write_bytes(bytes(restored.nor.bytes))
    stats_report=report(image_file)
    assert stats_report['project_slots']==12
    assert stats_report['sample_slots']==13
    assert stats_report['sector_overlap'] is False
    assert stats_report['projects_in_use']==11
    assert stats_report['samples_in_use']==13
    print('PASS: read-only local capacity report agrees with V2 storage contents')
print('V1.4.19 independent-storage coexistence tests COMPLETE')
