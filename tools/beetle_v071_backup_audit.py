#!/usr/bin/env python3
"""Read-only BEETLE TRKJ bank audit and TRK1 project extraction.

No USB/MIDI access. No flash erase/program. Accepts two 12288-byte raw
A/B bank images obtained via an independently verified reader, NOT an FM1
full-flash image (their mapping is hardware-specific and not inferred here).
"""
import argparse
import hashlib
import json
import struct
import sys
import zlib
from pathlib import Path

BANK_SIZE = 12288
HEADER_SIZE = 32
BANK_MAGIC = b'TRKJ'
RECORD_MAGIC = b'TRK1'
MAX_RECORD = 12256
SLOTS = 12
# Historical BEETLE-only proposed flash map. This does NOT prove physical
# ownership and never authorizes installation, erasure, or writes to a device.
FLASH_BYTES = 1048576
FLASH_BASE = 0x97000
MARKER_BASE = 0xDF000
MARKER_END = 0xE0000
MARKER_MAGIC = b'BTL33FS1'


def u16(data, at):
    return struct.unpack_from('<H', data, at)[0]


def u32(data, at):
    return struct.unpack_from('<I', data, at)[0]


def audit_bank(data, slot):
    """Return (classification, details, payload), strict and fail-closed."""
    if len(data) != BANK_SIZE:
        return 'INVALID', {'error': f'expected {BANK_SIZE} bytes, got {len(data)}'}, None
    if data == b'\xff' * BANK_SIZE:
        return 'BLANK', {}, None
    h = data[:HEADER_SIZE]
    if (h[:4] != BANK_MAGIC or u16(h, 4) != 1 or h[6] != slot or
        h[7] != (slot ^ 0xff) or u32(h, 24) != zlib.crc32(h[:20]) or
        h[20] not in (0, 0xff)):
        return 'FOREIGN', {'error': 'unrecognized journal ownership/header'}, None
    length = u32(h, 12)
    if length < 42 or length > MAX_RECORD or HEADER_SIZE + length > BANK_SIZE:
        return 'FOREIGN', {'error': 'out-of-range record length'}, None
    if any(v != 0xff for v in data[HEADER_SIZE+length:]):
        return 'FOREIGN', {'error': 'unexpected occupied bytes after payload'}, None
    base = {'generation': u32(h, 8), 'length': length, 'committed': h[20] == 0}
    if h[20] == 0xff:
        return 'INCOMPLETE', base, None
    payload = data[HEADER_SIZE:HEADER_SIZE+length]
    if zlib.crc32(payload) != u32(h, 16):
        return 'CORRUPT', {**base, 'error': 'journal payload CRC mismatch'}, None
    try:
        meta = verify_project(payload, slot)
    except ValueError as e:
        return 'CORRUPT', {**base, 'error': str(e)}, None
    return 'VALID', {**base, **meta}, payload


def verify_project(data, expected_slot=None):
    """Verify native TRK1 headers and CRCs, not the full semantic C decode."""
    if not 42 <= len(data) <= MAX_RECORD or data[:4] != RECORD_MAGIC:
        raise ValueError('invalid TRK1 signature or length')
    version = u16(data, 4)
    slot = u16(data, 6)
    if not 1 <= version <= 16 or slot >= SLOTS or (expected_slot is not None and slot != expected_slot):
        raise ValueError('unsupported native project version or slot mismatch')
    if u32(data, 8) != len(data) or u32(data, 16) != zlib.crc32(data[:16]) or u32(data, 12) != zlib.crc32(data[24:]):
        raise ValueError('native project header/contents CRC mismatch')
    if data[36] != slot:
        raise ValueError('payload slot disagrees with header slot')
    title = ''.join(chr(c) if 32 <= c <= 126 else ' ' for c in data[24:36]).rstrip()
    return {'slot': slot, 'schema': version, 'title': title, 'sha256': hashlib.sha256(data).hexdigest()}


def newest(a, b):
    """Mirror native 32-bit wrap-safe generation comparison."""
    delta = (b-a) & 0xffffffff
    return 0 < delta < 0x80000000


def audit_pair(slot, left, right):
    records = [audit_bank(left, slot), audit_bank(right, slot)]
    return records, {'slot': slot, 'bankA': {'status': records[0][0], **records[0][1]},
                     'bankB': {'status': records[1][0], **records[1][1]}}


def choose_export(records):
    # Mirror native conservative refusal of foreign occupancy.
    if any(r[0] in ('FOREIGN', 'INVALID') for r in records):
        raise ValueError('foreign/invalid bank detected: manual investigation required')
    valid = [n for n, v in enumerate(records) if v[0] == 'VALID']
    if not valid:
        raise ValueError('no independently CRC-valid committed project')
    if len(valid) == 2 and newest(records[0][1]['generation'],records[1][1]['generation']):
        return 1
    return valid[0]


def audit_full_dump(data):
    """Examine a user-supplied independent 1MiB dump; never contact hardware."""
    if len(data) != FLASH_BYTES:
        raise ValueError(f'exactly {FLASH_BYTES} bytes required, got {len(data)}')
    marker = data[MARKER_BASE:MARKER_END]
    expected = (MARKER_MAGIC + struct.pack('<IIII',FLASH_BASE,MARKER_BASE,BANK_SIZE,SLOTS))
    expected += struct.pack('<I',zlib.crc32(expected)) + b'\xff'*4
    if marker == b'\xff'*len(marker): marker_status = 'BLANK'
    elif marker[:32] == expected and marker[32:] == b'\xff'*(len(marker)-32):
        marker_status = 'BEETLE_MARKER_MATCH (NOT PROOF OF OWNERSHIP)'
    else: marker_status = 'FOREIGN_OR_DAMAGED'
    records = []
    for slot in range(SLOTS):
        off = FLASH_BASE + (slot*2)*BANK_SIZE
        left, right = data[off:off+BANK_SIZE], data[off+BANK_SIZE:off+2*BANK_SIZE]
        pair, report = audit_pair(slot,left,right)
        # A valid bank can be recovered offline even if its companion is torn.
        # Normal firmware save/load MUST continue refusing foreign banks.
        report['has_crc_valid_bank'] = any(v[0]=='VALID' for v in pair)
        records.append(report)
    return {'size':len(data),'layout':'BEETLE proposed 0x97000..0xDFFFF',
            'flash_ownership_is_not_certified':True,'marker':marker_status,
            'safe_to_flash':False,'slots':records}


def select_salvage_bank(records):
    """Salvage CRC-valid read-only payload; never use for erase authorization."""
    valid = [i for i, item in enumerate(records) if item[0]=='VALID']
    if not valid:
        raise ValueError('no committed bank with valid outer and TRK1 header/body CRC')
    if len(valid)==2:
        a,b = records[0][1],records[1][1]
        if a['generation']==b['generation'] and records[0][2]!=records[1][2]:
            raise ValueError('two divergent valid banks with equal generation: manual selection required')
        if newest(a['generation'],b['generation']): return 1
        if newest(b['generation'],a['generation']): return 0
        if a['generation']!=b['generation']:
            raise ValueError('ambiguous generation wrap: manual selection required')
    return valid[0]


def require_new_output(output, *inputs):
    if output.suffix.lower()!='.trk1' or output.resolve() in [p.resolve() for p in inputs]:
        raise ValueError('output must be a separate .trk1 file')
    if output.exists():
        raise ValueError('refusing to overwrite existing project backup')


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest='command',required=True)
    for command in ('audit','extract'):
        t=sub.add_parser(command)
        t.add_argument('--slot',type=int,required=True,choices=range(SLOTS),metavar='0..11')
        t.add_argument('--bank-a',type=Path,required=True)
        t.add_argument('--bank-b',type=Path,required=True)
        if command=='extract':t.add_argument('--output',type=Path,required=True,help='local .trk1 copy; never written to FM1')
    d=sub.add_parser('scan-dump',help='READ ONLY: inspect an independently obtained exact 1MiB dump')
    d.add_argument('dump',type=Path)
    d=sub.add_parser('salvage-dump',help='READ ONLY: copy a CRC-valid TRK1 record from a dump')
    d.add_argument('dump',type=Path)
    d.add_argument('--slot',type=int,required=True,choices=range(SLOTS),metavar='0..11')
    d.add_argument('--output',type=Path,required=True)
    d.add_argument('--acknowledge-readonly-salvage',action='store_true',help='acknowledge this is a CRC-only backup, not a verified device restore')
    t=sub.add_parser('verify')
    t.add_argument('file',type=Path,help='previously extracted TRK1 file')
    args=p.parse_args(argv)
    if args.command in ('scan-dump','salvage-dump'):
        try:
            data=args.dump.read_bytes()
            report=audit_full_dump(data)
            if args.command=='scan-dump':
                print(json.dumps(report,indent=2));return 0
            if not args.acknowledge_readonly_salvage:
                p.error('add --acknowledge-readonly-salvage; no FM1 restore is performed')
            require_new_output(args.output,args.dump)
            off=FLASH_BASE+args.slot*2*BANK_SIZE
            pair,_=audit_pair(args.slot,data[off:off+BANK_SIZE],data[off+BANK_SIZE:off+2*BANK_SIZE])
            which=select_salvage_bank(pair)
            payload=pair[which][2]
            # Open exclusively: refuse concurrent/pre-existing outputs, never overwrite.
            with args.output.open('xb') as handle:handle.write(payload)
            print(json.dumps({'result':'READ-ONLY CRC-VALID SALVAGE (NOT A FIRMWARE RESTORE)',
                'slot':args.slot,'selected_bank':'AB'[which],
                'companion_status':pair[1-which][0],
                'sha256':hashlib.sha256(payload).hexdigest(),
                'native_project_semantics_verified':False,
                'source_dump_modified':False,'file':str(args.output)},indent=2));return 0
        except (ValueError,OSError) as e:p.error(str(e))
    if args.command=='verify':
        try:result=verify_project(args.file.read_bytes())
        except ValueError as e: p.error(str(e))
        print(json.dumps({'result':'VALID TRK1 HEADER/CRC','file':str(args.file),**result},indent=2));return 0
    try:
        records,report=audit_pair(args.slot,args.bank_a.read_bytes(),args.bank_b.read_bytes())
    except OSError as e:
        p.error(str(e))
    if args.command=='audit':
        print(json.dumps(report,indent=2));return 0
    try:active=choose_export(records)
    except ValueError as e:
        print(json.dumps(report,indent=2));p.error(str(e))
    contents=records[active][2]
    if args.output.suffix.lower()!='.trk1' or args.output.resolve() in (args.bank_a.resolve(),args.bank_b.resolve()):
        p.error('destination must be a separate .trk1 file')
    if args.output.exists():p.error('refusing to overwrite an existing backup')
    with args.output.open('xb') as handle:handle.write(contents)
    print(json.dumps({'result':'EXTRACTED READ-ONLY','selectedBank':'AB'[active],
                      'file':str(args.output),**report},indent=2));return 0


if __name__=='__main__':sys.exit(main())
