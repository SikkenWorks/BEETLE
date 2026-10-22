#!/usr/bin/env python3
"""Exact source-to-V2 migration image and protected-OTA byte identity tests."""
import sys,struct,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/storage_v2'))
sys.path.insert(0,str(ROOT/'tools/storage_v2'))
from test_legacy_flash_audit import project,bank
from legacy_flash_audit import BASE,END,MARKER,FLASH_BYTES,expected_marker
from create_v2_recovery_image import make_image,marker
from migration_preflight import MigrationError
from storage_v2_sim import Nor,Storage,ProjectCodec
import ctypes,subprocess,tempfile
with tempfile.TemporaryDirectory() as folder:
    source=bytearray(b'\xff'*FLASH_BYTES)
    source[MARKER:MARKER+32]=expected_marker()
    a=(ROOT/'tests/fixtures/NUGGIE-SLOT-07.trk').read_bytes()
    b=(ROOT/'tests/fixtures/MOUNTAINKING-SLOT-01.trk').read_bytes()
    bank(source,0,0,project(0,a),2)
    bank(source,3,1,project(3,b),8)
    projects={i:None for i in range(1,13)}
    projects[1]=project(0,a);projects[4]=project(3,b)
    samples={1:bytes([0x35])*4500,13:bytes([0x12])*2000}
    converted,report=make_image(bytes(source),projects,samples)
    assert len(converted)==len(source)==1048576
    assert converted[:BASE]==source[:BASE]
    assert converted[0xE0000:]==source[0xE0000:]
    assert converted[MARKER:MARKER+32]==marker()
    assert converted[BASE:END]!=source[BASE:END]
    assert report['user_samples']==2 and report['projects_migrated']==2
    bad=dict(projects);bad[1]=b
    try:make_image(bytes(source),bad,samples)
    except MigrationError:pass
    else:raise AssertionError('incorrect backup accepted')
    print('PASS: authenticated legacy 12-slot migration into byte-exact V2 partition; boot/OTA/data outside BEETLE region untouched, wrong backup refused')
