#!/usr/bin/env python3
"""Synthetic read-only full-flash audit and V2 preflight. Never accesses hardware."""
from __future__ import annotations
from pathlib import Path
import copy
import hashlib
import json
import struct
import sys
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/storage_v2'))
from legacy_flash_audit import (FLASH_BYTES, BASE, END, BANK_BYTES, HEAD_BYTES,
    MARKER, MARKER_END, AuditError, audit_image, export_projects, expected_marker, crc)
from migration_preflight import simulate


def project(slot, origin):
    b = bytearray(origin)
    struct.pack_into('<H', b, 6, slot)
    struct.pack_into('<I', b, 16, crc(b[:16]))
    assert crc(b[24:]) == struct.unpack_from('<I', b, 12)[0]
    return bytes(b)

def bank(image, slot, bank_id, raw, gen, *, committed=True):
    off = BASE + (slot * 2 + bank_id) * BANK_BYTES
    h = bytearray(b'\xff'*HEAD_BYTES)
    h[:4] = b'TRKJ'
    struct.pack_into('<H', h, 4, 1)
    h[6:8] = bytes((slot, slot ^ 255))
    struct.pack_into('<III', h, 8, gen, len(raw), crc(raw))
    h[20] = 0 if committed else 0xff
    struct.pack_into('<I', h, 24, crc(h[:20]))
    image[off:off+HEAD_BYTES] = h
    image[off+HEAD_BYTES:off+HEAD_BYTES+len(raw)] = raw
    return off

def reject(image, reason):
    try: audit_image(bytes(image))
    except AuditError: return
    raise AssertionError('invalid flash accepted: '+reason)

def run():
    nuggie = (ROOT/'tests/fixtures/NUGGIE-SLOT-07.trk').read_bytes()
    mountain = (ROOT/'tests/fixtures/MOUNTAINKING-SLOT-01.trk').read_bytes()
    image = bytearray(b'\xff'*FLASH_BYTES)
    image[MARKER:MARKER+32] = expected_marker()
    old = project(0,mountain)
    new = project(0,nuggie)
    at0 = bank(image,0,0,old,0xfffffffE)
    at1 = bank(image,0,1,new,1)  # wraps around signed difference
    bank(image,6,0,project(6,nuggie),13)
    report, files = audit_image(bytes(image))
    assert report['projects_recovered']==2 and report['empty_slots']==10
    assert files[1]==new and files[7]==project(6,nuggie)
    assert report['slots'][0]['chosen_bank']=='B'
    assert report['slots'][0]['generation']==1
    assert 'payload' not in json.dumps(report).lower()
    image_hash = hashlib.sha256(image).digest()
    with tempfile.TemporaryDirectory(prefix='beetle-audit-') as tmp:
        folder=Path(tmp)/'export'
        export_projects(folder,bytes(image))
        manifest=json.loads((folder/'MIGRATION-MANIFEST.json').read_text())
        assert len(manifest['projects'])==12
        assert manifest['projects'][0]['status']=='file'
        assert manifest['projects'][1]['status']=='empty'
        assert (folder/'SLOT-01-verified.trk').read_bytes()==new
        assert simulate(folder/'MIGRATION-MANIFEST.json')['verified_after_simulated_reboot']
        try: export_projects(folder,bytes(image))
        except AuditError: pass
        else: raise AssertionError('overwrote existing export')
    assert hashlib.sha256(image).digest()==image_hash
    # CRC failure in newer bank must use older committed generation.
    damaged=bytearray(image)
    damaged[at1+HEAD_BYTES+36] ^= 0x01
    recovered, files = audit_image(bytes(damaged))
    assert files[1]==old and recovered['slots'][0]['damaged_sister_bank']
    # Both generations damaged: cannot guess a project, no exports.
    damaged[at0+HEAD_BYTES+35] ^= 0x01
    reject(damaged,'no valid A/B generation')
    # Erased first 32 bytes but payload survives: foreign, not empty.
    foreign=bytearray(image)
    off = BASE + (1*2)*BANK_BYTES
    foreign[off+100] = 0
    reject(foreign,'erased header with occupied tail')
    # Unexpected after declared data / unknown owner must never be reclaimed.
    foreign=bytearray(image)
    foreign[at1+HEAD_BYTES+len(new)+4]=0
    reject(foreign,'non-FF journal tail')
    foreign=bytearray(image)
    foreign[at1]=0
    reject(foreign,'foreign bank magic')
    foreign=bytearray(image)
    foreign[MARKER+200]=0
    reject(foreign,'occupied marker tail')
    foreign=bytearray(image)
    foreign[MARKER]=0
    reject(foreign,'foreign marker')
    reject(image[:-1],'truncated full flash dump')
    reject(image+b'\xff','oversized full flash dump')
    # Committed twin banks with same generation but divergent data are ambiguous.
    tie=bytearray(image)
    bank(tie,0,0,old,1)
    reject(tie,'conflicting equal generation')
    # Version >24 rejected despite a matching outer CRC.
    invalid=bytearray(image)
    p=bytearray(new);struct.pack_into('<H',p,4,25)
    struct.pack_into('<I',p,16,crc(p[:16]))
    bank(invalid,0,1,bytes(p),20)
    # Older valid bank retained after rejecting newer incompatible version.
    report, files = audit_image(bytes(invalid))
    assert files[1] == old and report['slots'][0]['damaged_sister_bank']
    # Incomplete write cannot become committed just because its payload CRC is correct.
    incomplete=bytearray(image)
    bank(incomplete,0,1,new,20,committed=False)
    report, files=audit_image(bytes(incomplete))
    assert files[1]==old and report['slots'][0]['damaged_sister_bank']
    # Never make a directory of project data on a rejected image.
    with tempfile.TemporaryDirectory(prefix='beetle-audit-fail-') as tmp:
        folder=Path(tmp)/'bad'
        try: export_projects(folder,bytes(damaged))
        except AuditError: pass
        else: raise AssertionError('exported incomplete image')
        assert not folder.exists()
    print('PASS V1.4.17 legacy flash read-only auditor: full ownership, 24 banks, generation wrap, CRC fallback, manifest -> Storage V2 reboot, 11 rejection cases')

if __name__=='__main__': run()
