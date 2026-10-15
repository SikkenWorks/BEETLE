#!/usr/bin/env python3
"""Read-only, no device access, 1MiB full-flash qualification fixtures."""
from pathlib import Path
import importlib.util, io, struct, tempfile, zlib, hashlib
from contextlib import redirect_stdout
root=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('audit076',root/'tools/beetle_v071_backup_audit.py')
a=importlib.util.module_from_spec(spec);spec.loader.exec_module(a)

def payload(slot=0, title=b'VALID   TEST'):
    r=bytearray(42);r[:4]=b'TRK1';struct.pack_into('<HH',r,4,16,slot)
    r[24:36]=title.ljust(12,b' ');r[36]=slot
    struct.pack_into('<I',r,8,len(r));struct.pack_into('<I',r,12,zlib.crc32(r[24:]))
    struct.pack_into('<I',r,16,zlib.crc32(r[:16]));return bytes(r)

def bank(record,slot=0,generation=1):
    h=bytearray(b'\xff'*a.BANK_SIZE);h[:4]=b'TRKJ'
    struct.pack_into('<HBBIII',h,4,1,slot,slot^255,generation,len(record),zlib.crc32(record))
    h[20]=0;struct.pack_into('<I',h,24,zlib.crc32(h[:20]))
    h[32:32+len(record)]=record;return bytes(h)

def slot_bank(data,slot,bank_index,contents):
    off=a.FLASH_BASE+(slot*2+bank_index)*a.BANK_SIZE
    data[off:off+a.BANK_SIZE]=contents

def make_marker():
    e=a.MARKER_MAGIC+struct.pack('<IIII',a.FLASH_BASE,a.MARKER_BASE,a.BANK_SIZE,a.SLOTS)
    return e+struct.pack('<I',zlib.crc32(e))+b'\xff'*4

with tempfile.TemporaryDirectory() as td:
    td=Path(td);src=td/'private_dump.bin';dst=td/'recovered.trk1'
    content=bytearray(b'\xff'*a.FLASH_BYTES)
    content[a.MARKER_BASE:a.MARKER_BASE+32]=make_marker()
    p=payload();q=payload(title=b'OLDER\x20\x20\x20\x20\x20\x20\x20')
    slot_bank(content,0,0,bank(p,0,7))
    foreign=bytearray(bank(q,0,8));foreign[0]=ord('X')
    slot_bank(content,0,1,foreign)
    # A different slot must remain invisible to chosen-slot extraction.
    slot_bank(content,1,0,bank(payload(slot=1),1,1))
    src.write_bytes(content);original_hash=hashlib.sha256(src.read_bytes()).hexdigest()
    summary=a.audit_full_dump(bytes(content))
    assert summary['marker'].startswith('BEETLE_MARKER_MATCH')
    assert summary['safe_to_flash'] is False
    assert summary['slots'][0]['bankA']['status']=='VALID'
    assert summary['slots'][0]['bankB']['status']=='FOREIGN'
    assert summary['slots'][1]['bankA']['status']=='VALID'
    assert summary['slots'][2]['bankA']['status']=='BLANK'
    pair,_=a.audit_pair(0,bank(p,0,7),bytes(foreign))
    try:a.choose_export(pair);assert False
    except ValueError:pass
    assert a.select_salvage_bank(pair)==0
    # Never permit output without explicit acknowledgement.
    try:
        with redirect_stdout(io.StringIO()):a.main(['salvage-dump',str(src),'--slot','0','--output',str(dst)])
        assert False
    except SystemExit:pass
    assert not dst.exists()
    with redirect_stdout(io.StringIO()):
        assert a.main(['scan-dump',str(src)])==0
        assert a.main(['salvage-dump',str(src),'--slot','0','--output',str(dst),'--acknowledge-readonly-salvage'])==0
    assert dst.read_bytes()==p
    assert hashlib.sha256(src.read_bytes()).hexdigest()==original_hash
    try:
        with redirect_stdout(io.StringIO()):
            a.main(['salvage-dump',str(src),'--slot','0','--output',str(dst),'--acknowledge-readonly-salvage'])
        assert False
    except SystemExit:pass
    assert dst.read_bytes()==p
    torn=bytearray(content);torn[a.MARKER_BASE+4]^=3
    assert a.audit_full_dump(torn)['marker']=='FOREIGN_OR_DAMAGED'
    for illegal in (b'',b'\xff'*(a.FLASH_BYTES-1),b'\xff'*(a.FLASH_BYTES+1)):
        try:a.audit_full_dump(illegal);assert False
        except ValueError:pass
    # Split-brain equal sequence must never choose one bank arbitrarily.
    pair,_=a.audit_pair(0,bank(p,0,10),bank(q,0,10))
    try:a.select_salvage_bank(pair);assert False
    except ValueError:pass
    # A higher wrapping generation is recognised.
    pair,_=a.audit_pair(0,bank(p,0,0xffffffff),bank(q,0,0))
    assert a.select_salvage_bank(pair)==1
    # Torn payload must be rejected even with correct outer CRC.
    invalid=bytearray(bank(p));invalid[32]^=1
    assert a.audit_bank(invalid,0)[0]=='CORRUPT'
    print('PASS V0.76 full NOR dump scan: 12 slots, strict 1MiB and marker, guarded read-only salvage, divergent generation and input integrity')
