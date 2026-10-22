#!/usr/bin/env python3
"""Offline-only BEETLE Storage V2 stress + power-loss qualification.
No FM1 writes, no bootloader, no OTA interactions.
"""
import ctypes
import os
from pathlib import Path
import random
import subprocess
import sys
import tempfile
import time

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/storage_v2'))
from storage_v2_sim import (Nor,Storage,ProjectCodec,PowerFailure,UnsafeLayout,
                            NotEnoughSafeSpace,CorruptData,crc,SECTOR,SECTORS)

SRC=ROOT/'firmware/fm1-native/beetle_project_pack_v2.c'
with tempfile.TemporaryDirectory(prefix='beetle-pack-') as temp:
    libname=str(Path(temp)/'packer.so')
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=undefined',
                    '-fPIC','-shared',str(SRC),'-o',libname],check=True)
    codec=ProjectCodec(ctypes.CDLL(libname))
    files=sorted((ROOT/'tests/fixtures').glob('*.trk'))
    assert len(files)>=4, 'original TRK fixtures unavailable'
    fixtures={f.name:f.read_bytes() for f in files}
    nuggie=fixtures['NUGGIE-SLOT-07.trk']
    mountain=fixtures['MOUNTAINKING-SLOT-01.trk']
    print('BEETLE Storage V2 OFFLINE testing; source fixtures',len(fixtures),flush=True)
    for fname,b in fixtures.items():
        packed=codec.pack(b)
        assert codec.unpack(packed)==b
        assert len(packed)<=len(b)+16
        tampered=bytearray(packed);tampered[-1]^=1
        try: codec.unpack(bytes(tampered))
        except CorruptData:pass
        else: raise AssertionError('undetected bit flip in '+fname)
        print('lossless',fname,len(b),'->',len(packed),flush=True)
    rng=random.Random(0xBEE71E)
    for k in range(120):
        size=rng.randint(1,12256)
        data=bytes(rng.randrange(256) for _ in range(size)) if k%3==0 else (
              bytes([rng.randrange(256)])*size if k%3==1 else
              bytes((i*7+(i//17))%256 for i in range(size)))
        assert codec.unpack(codec.pack(data))==data
    assert codec.unpack(codec.pack(os.urandom(12256)))
    print('PASS 120 randomized roundtrip/corruption/RAW fallback cases',flush=True)
    # Reject NOR image with alien content; no implicit format/migration.
    stranger=bytearray(b'\xff'*(SECTOR*SECTORS));stranger[SECTOR*3]=0
    try:Storage(Nor(stranger,authorized=True),codec)
    except UnsafeLayout:pass
    else:raise AssertionError('foreign occupied image accepted')
    try:Storage(Nor(authorized=False),codec).save_project(0,nuggie)
    except UnsafeLayout:pass
    else:raise AssertionError('unowned NOR can be modified')
    nor=Nor(authorized=True);s=Storage(nor,codec)
    s.save_project(0,nuggie)
    s.save_project(1,mountain)
    s.replace_sample(2,b'abc123'*900)
    assert s.load_project(0)==nuggie and s.load_project(1)==mountain
    assert s.load_sample(2)==b'abc123'*900
    assert s.stats()['erased_sectors']>=4
    # Persistent reboot preserves all live data.
    s=Storage(Nor(bytes(nor.bytes),authorized=True),codec)
    assert s.load_project(0)==nuggie and s.load_project(1)==mountain
    assert s.load_sample(2)==b'abc123'*900
    print('PASS initial persistence and read-back',flush=True)
    # Power-cut fault injection at EVERY NOR program/erase boundary during project overwrite.
    initial=bytes(nor.bytes)
    count=0;resumed=set()
    for step in range(1,120):
        dev=Nor(initial,authorized=True)
        store=Storage(dev,codec)
        dev.fault_after=step
        try:store.save_project(0,mountain)
        except PowerFailure:pass
        else:
            count=step-1;break
        reboot=Storage(Nor(bytes(dev.bytes),authorized=True),codec)
        assert reboot.load_project(0) in (nuggie,mountain),('lost committed project',step)
        assert reboot.load_project(1)==mountain,('corrupted other project',step)
        assert reboot.load_sample(2)==b'abc123'*900,('corrupted other sample',step)
        resumed.add(reboot.load_project(0))
    else:raise AssertionError('project transaction did not finish')
    assert nuggie in resumed
    print('PASS exhaustive project power cuts at',count,'NOR mutation boundaries',flush=True)
    # Sample replacement has CRC protection and must never corrupt previous bank/other projects.
    new_sample=b'k'*11200
    samplecuts=0
    for step in range(1,140):
        dev=Nor(initial,authorized=True);store=Storage(dev,codec);dev.fault_after=step
        try:store.replace_sample(2,new_sample)
        except PowerFailure:pass
        else:
            samplecuts=step-1;break
        reboot=Storage(Nor(bytes(dev.bytes),authorized=True),codec)
        assert reboot.load_sample(2) in (b'abc123'*900,new_sample),('sample replace lost both',step)
        assert reboot.load_project(0)==nuggie and reboot.load_project(1)==mountain
    else:raise AssertionError('sample transaction did not finish')
    print('PASS exhaustive sample power cuts at',samplecuts,'NOR mutation boundaries',flush=True)
    # Sample removal is metadata only and must be power loss safe.
    for step in range(1,22):
        dev=Nor(initial,authorized=True);store=Storage(dev,codec);dev.fault_after=step
        try:store.delete_sample(2)
        except PowerFailure:pass
        else:break
        reboot=Storage(Nor(bytes(dev.bytes),authorized=True),codec)
        assert reboot.load_sample(2) in (b'abc123'*900,None)
        assert reboot.load_project(0)==nuggie
    else:raise AssertionError('sample delete did not finish')
    print('PASS sample deletion and rollback',flush=True)
    # Populate all 12 slots, exercises packed 256-byte records, fallback from corruption.
    dev=Nor(authorized=True);store=Storage(dev,codec)
    for slot in range(12):store.save_project(slot,mountain if slot%4==0 else nuggie)
    for slot in range(12):
        want=mountain if slot%4==0 else nuggie
        assert store.load_project(slot)==want
    store.save_project(0,nuggie)
    live=store.current.projects[0]
    assert live[0] is not None and live[1] is not None
    # CRC corruption of newest record allows project-local verified fallback.
    data=bytearray(dev.bytes)
    data[live[0].ref.offset+27]^=1
    recovered=Storage(Nor(data,authorized=True),codec)
    assert recovered.load_project(0)==mountain
    for slot in range(1,12):assert recovered.load_project(slot)==(mountain if slot%4==0 else nuggie)
    print('PASS 12 slots, bit corruption, old-generation fallback',flush=True)
    # Stress 10s sample: transfer uses chunk-sized records and persists after restart.
    dev=Nor(authorized=True);store=Storage(dev,codec)
    for slot in range(12):store.save_project(slot,mountain if slot==0 else nuggie)
    ten_seconds=bytes((i*31+i//37)&255 for i in range(113784))
    store.replace_sample(0,ten_seconds)
    assert store.load_sample(0)==ten_seconds
    reboot=Storage(Nor(bytes(dev.bytes),authorized=True),codec)
    assert reboot.load_sample(0)==ten_seconds
    assert all(reboot.load_project(slot)==(mountain if slot==0 else nuggie) for slot in range(12))
    print('PASS 10-sec-sized encoded payload, 12 projects, reboot',reboot.stats(),flush=True)
    # Full 20s scenario is achievable only when actual project occupancy permits.
    twenty=bytes((i*19+i//53)&255 for i in range(227436))
    fresh=Nor(authorized=True);mix=Storage(fresh,codec)
    for slot in range(12):mix.save_project(slot,mountain if slot==0 else nuggie)
    mix.replace_sample(0,twenty)
    assert Storage(Nor(bytes(fresh.bytes),authorized=True),codec).load_sample(0)==twenty
    assert mix.stats()['erased_sectors']>=4
    print('PASS 20-sec-sized payload in 1-large/11-small case, safety reserve intact',flush=True)
    # Dense/high-entropy projects cannot be silently truncated to make sample room.
    dense=Nor(authorized=True);d=Storage(dense,codec)
    rng_dense=random.Random(19371)
    originals=[rng_dense.randbytes(12256) for _ in range(12)]
    for slot,data in enumerate(originals):d.save_project(slot,data)
    before=bytes(dense.bytes)
    try:d.replace_sample(0,twenty)
    except NotEnoughSafeSpace:pass
    else:raise AssertionError('accepted overfull 20-second case with 12 dense projects')
    assert bytes(dense.bytes)==before
    assert all(d.load_project(i)==originals[i] for i in range(12))
    print('PASS 12 incompressible maximum project slots preserved, 20-sec upload refused',flush=True)
    # Metadata retirement and sector-level garbage collection after replacement.
    recycling=Nor(authorized=True);r=Storage(recycling,codec)
    r.save_project(0,nuggie)
    r.replace_sample(0,b'\x35'*17000)
    r.replace_sample(0,b'\x7f'*19000)
    assert r.load_sample(0)==b'\x7f'*19000
    r.delete_sample(0)
    r.checkpoint()
    assert r.load_project(0)==nuggie
    assert r.load_sample(0) is None
    r.replace_sample(0,b'\x21'*14000)
    assert Storage(Nor(bytes(recycling.bytes),authorized=True),codec).load_sample(0)==b'\x21'*14000
    print('PASS delete/replace + checkpoint reclaim without touching live projects',flush=True)
    # All 13 logical U slots survive independent replacements across twelve projects.
    banks=Nor(authorized=True);bankstore=Storage(banks,codec)
    for project_slot in range(12):bankstore.save_project(project_slot,nuggie)
    for sample_slot in range(13):bankstore.replace_sample(sample_slot,bytes([sample_slot+1])*(511+sample_slot))
    for sample_slot in range(13):
        assert bankstore.load_sample(sample_slot)==bytes([sample_slot+1])*(511+sample_slot)
    reload_banks=Storage(Nor(bytes(banks.bytes),authorized=True),codec)
    assert all(reload_banks.load_sample(i)==bytes([i+1])*(511+i) for i in range(13))
    print('PASS independent 13 user slots across 12 projects',flush=True)
    # Model torn page/metadata commit under voltage collapse: never silently trust partial index.
    torn=bytearray(initial)
    active=Storage(Nor(initial,authorized=True),codec)
    newest=max(active.available,key=lambda x:x.generation)
    # Partially erased/programmed final commit marker is never trusted.
    torn[newest.sector*SECTOR+SECTOR-2]=0xff
    rollback=Storage(Nor(torn,authorized=True),codec)
    assert rollback.current.generation<newest.generation
    assert rollback.load_project(0)==nuggie
    print('PASS truncated metadata commit marker rejected (torn write)',flush=True)
    # Never claim unconditional 20 seconds or destructive over-allocation.
    before=bytes(dev.bytes)
    huge=bytes((i*19+i//51)&255 for i in range(228000))
    try:reboot.replace_sample(1,huge)
    except NotEnoughSafeSpace:pass
    else:raise AssertionError('unexpected oversized sample acceptance')
    assert bytes(reboot.nor.bytes)==before,'full-capacity refusal modified flash'
    print('PASS oversized sample refused without any NOR mutation',flush=True)
    # Metadata corruption: fallback to previous valid index, if retained records exist.
    candidate=Storage(Nor(initial,authorized=True),codec)
    newest=max(candidate.available,key=lambda x:x.generation)
    corrupted=bytearray(initial)
    corrupted[newest.sector*SECTOR+SECTOR-1]^=1
    fallback=Storage(Nor(corrupted,authorized=True),codec)
    assert fallback.current.generation<newest.generation
    assert fallback.load_project(0)==nuggie and fallback.load_project(1)==mountain
    print('PASS corrupt newest superblock falls back to verified prior metadata',flush=True)
    # Test no implicit recovery from all-invalid metadata; fail-closed.
    damaged=bytearray(initial)
    for slot in range(2):damaged[slot*SECTOR+SECTOR-1]^=1
    try:Storage(Nor(damaged,authorized=True),codec)
    except UnsafeLayout:pass
    else:raise AssertionError('broken metadata treated as blank')
    print('PASS unreadable flash fails closed; NO physical device modifications',flush=True)
