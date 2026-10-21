#!/usr/bin/env python3
"""Host-only read-only native raw bank backup inspector qualification."""
from pathlib import Path
import importlib.util
import struct
import sys
import tempfile
import zlib
root=Path(__file__).resolve().parents[2]
script=root/'tools/beetle_v071_backup_audit.py'
spec=importlib.util.spec_from_file_location('audit071',script)
m=importlib.util.module_from_spec(spec);sys.modules[spec.name]=m;spec.loader.exec_module(m)

def make_payload(slot=0,bpm=120):
    data=bytearray(42)
    data[:4]=b'TRK1';struct.pack_into('<H',data,4,16);struct.pack_into('<H',data,6,slot)
    data[24:36]=b'BEETLE BACKU';data[36]=slot
    data[37:40]=bytes([bpm,0,0]);struct.pack_into('<I',data,8,len(data))
    struct.pack_into('<I',data,12,zlib.crc32(data[24:]));struct.pack_into('<I',data,16,zlib.crc32(data[:16]))
    return bytes(data)

def bank(payload=None,gen=1,slot=0,committed=True):
    if payload is None:return bytes([255])*m.BANK_SIZE
    b=bytearray([255]*m.BANK_SIZE)
    h=bytearray([255]*m.HEADER_SIZE)
    h[:4]=b'TRKJ';struct.pack_into('<H',h,4,1)
    h[6]=slot;h[7]=slot^255
    struct.pack_into('<I',h,8,gen);struct.pack_into('<I',h,12,len(payload))
    struct.pack_into('<I',h,16,zlib.crc32(payload))
    h[20]=0 if committed else 255
    struct.pack_into('<I',h,24,zlib.crc32(h[:20]))
    b[:32]=h;b[32:32+len(payload)]=payload
    return bytes(b)

p=make_payload()
assert m.verify_project(p,0)['schema']==16
try:m.verify_project(p,1);assert False
except ValueError:pass
b=bank(p)
assert m.audit_bank(b,0)[0]=='VALID'
assert m.audit_bank(bank(p,committed=False),0)[0]=='INCOMPLETE'
assert m.audit_bank(bank(),0)[0]=='BLANK'
corrupt=bytearray(b);corrupt[40]^=1
assert m.audit_bank(bytes(corrupt),0)[0]=='CORRUPT'
foreign=bytearray(b);foreign[0]=ord('F')
assert m.audit_bank(bytes(foreign),0)[0]=='FOREIGN'
assert m.choose_export([m.audit_bank(bank(p,gen=4294967295),0),m.audit_bank(bank(p,gen=0),0)])==1
assert m.choose_export([m.audit_bank(b,0),m.audit_bank(bank(),0)])==0
try:m.choose_export([m.audit_bank(b,0),m.audit_bank(bytes(foreign),0)]);assert False
except ValueError:pass
with tempfile.TemporaryDirectory() as td:
    a=Path(td)/'a.bin';c=Path(td)/'b.bin';out=Path(td)/'project.trk1'
    a.write_bytes(b);c.write_bytes(bank())
    assert m.main(['audit','--slot','0','--bank-a',str(a),'--bank-b',str(c)])==0
    assert m.main(['extract','--slot','0','--bank-a',str(a),'--bank-b',str(c),'--output',str(out)])==0
    assert out.read_bytes()==p
    assert m.main(['verify',str(out)])==0
    try:m.main(['extract','--slot','0','--bank-a',str(a),'--bank-b',str(c),'--output',str(out)]);assert False
    except SystemExit:pass
print('PASS V0.71 native bank read-only audit, rollback, CRC, foreign, no-overwrite and extraction')
