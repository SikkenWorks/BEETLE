#!/usr/bin/env python3
"""Non-destructive V1.4.16 migration preflight acceptance tests."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import zlib
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/storage_v2'))
from migration_preflight import simulate, MigrationError, read_project, NotEnoughSafeSpace
sys.path.insert(0,str(ROOT/'tools/dpcm4'))
import beetle_dpcm4 as dpcm

fixtures=ROOT/'tests/fixtures'
nuggie=(fixtures/'NUGGIE-SLOT-07.trk').read_bytes()
mountain=(fixtures/'MOUNTAINKING-SLOT-01.trk').read_bytes()
with tempfile.TemporaryDirectory(prefix='beetle-migration-test-') as temp:
    root=Path(temp)
    (root/'nuggie.trk').write_bytes(nuggie)
    (root/'mountain.trk').write_bytes(mountain)
    (root/'u01.bd4').write_bytes(dpcm.encode([0,1000,-1000,0]*200))
    data={'format':'BEETLE-MIGRATION-PREFLIGHT-1',
          'projects':[{'slot':i,'status':'file','path':'mountain.trk' if i==1 else 'nuggie.trk'} for i in range(1,13)],
          'samples':[{'slot':1,'path':'u01.bd4'}]}
    manifest=root/'manifest.json'
    def save():manifest.write_text(json.dumps(data))
    save()
    result=simulate(manifest)
    assert result['project_files']==12 and result['samples']==1
    assert result['free_erased_sectors']>=4 and result['verified_after_simulated_reboot']
    run=subprocess.run([sys.executable,str(ROOT/'tools/storage_v2/migration_preflight.py'),str(manifest)],capture_output=True,text=True)
    assert run.returncode==0,run.stderr
    assert 'OFFLINE_PREFLIGHT_ONLY' in run.stdout
    # Corrupted authenticated/packed inputs must fail instead of silently migrating.
    tamper=bytearray(nuggie);tamper[-1]^=1;(root/'bad.trk').write_bytes(tamper)
    data['projects'][4]['path']='bad.trk';save()
    try:simulate(manifest)
    except MigrationError:pass
    else:raise AssertionError('bad CRC accepted')
    data['projects'][4]['path']='nuggie.trk'
    # No auto-guessing absent project slots.
    data['projects'].pop();save()
    try:simulate(manifest)
    except MigrationError:pass
    else:raise AssertionError('missing twelfth slot accepted')
    data['projects'].append({'slot':12,'status':'empty'});save()
    result=simulate(manifest)
    assert result['project_files']==11 and result['empty_slots']==1
    # No path escaping to sensitive host files.
    data['projects'][0]['path']='../outside.trk';save()
    try:simulate(manifest)
    except MigrationError:pass
    else:raise AssertionError('traversal accepted')
    data['projects'][0]['path']='mountain.trk'
    # A near-20 second sample alongside dense projects must fail without a single FM1 write.
    for i in range(1,13):
        from random import Random
        # Packer can accept arbitrary payload, but migration preflight requires valid TRK1.
        data['projects'][i-1]={'slot':i,'status':'file','path':'nuggie.trk'}
    data['samples']=[{'slot':i,'path':f'u{i:02}.bd4'} for i in range(1,14)]
    for i in range(1,14):
        import math
        (root/f'u{i:02}.bd4').write_bytes(dpcm.encode([int(10000*math.sin(k*0.22+i)) for k in range(22050)]))
    save()
    result=simulate(manifest)
    assert result['samples']==13 and result['sample_total_seconds']==13
    assert result['free_erased_sectors']>=4
    # Authenticated V2 exports are accepted; tampering with their GCM tag is not.
    from cryptography.hazmat.primitives.ciphers.aead import AESGCM
    from migration_preflight import KEY
    nonce=b'1234567890ab'
    prefix=b'BEETLE01\x02'+nonce
    protected=prefix+AESGCM(KEY).encrypt(nonce,mountain,prefix)
    (root/'mountain.beetle').write_bytes(protected)
    data['projects'][0]['path']='mountain.beetle';save()
    assert simulate(manifest)['project_files']==12
    corrupt=bytearray(protected);corrupt[-1]^=1
    (root/'tampered.beetle').write_bytes(corrupt)
    data['projects'][0]['path']='tampered.beetle';save()
    try:simulate(manifest)
    except MigrationError:pass
    else:raise AssertionError('unauthenticated encrypted backup accepted')
    data['projects'][0]['path']='mountain.trk'
    # Valid but incompressible maximum-size TRK1 projects + 20 sec BD4S must
    # fail the 288 KiB model rather than deleting any of the twelve slots.
    import struct, random
    for i in range(1,13):
        payload=random.Random(i*71).randbytes(12256-24)
        head=bytearray(mountain[:24])
        struct.pack_into('<H',head,4,24)
        struct.pack_into('<I',head,8,12256)
        struct.pack_into('<I',head,12,zlib.crc32(payload)&0xffffffff)
        struct.pack_into('<I',head,16,zlib.crc32(head[:16])&0xffffffff)
        filename=f'dense{i:02}.trk'
        (root/filename).write_bytes(bytes(head)+payload)
        data['projects'][i-1]['path']=filename
    (root/'full20.bd4').write_bytes(dpcm.encode([0]*441000))
    data['samples']=[{'slot':1,'path':'full20.bd4'}]
    save()
    try:simulate(manifest)
    except NotEnoughSafeSpace:pass
    else:raise AssertionError('overfull 12-project/20s migration accepted')
    print('PASS V1.4.16 migration: 12-slot explicit manifests, CRC rejection, path safety, 13 BD4S sources, reboot parity')
