#!/usr/bin/env python3
"""V1.4.24 native migration bootstrap index must match the established V2 format."""
from pathlib import Path
import sys,zlib,struct
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/storage_v2'))
from storage_v2_sim import Snapshot, _snapshot_bytes
b=bytearray(b'\xff'*4096)
b[:20]=b'\x00'*20;b[:4]=b'B2SB';b[4]=1;b[8]=1
payload=12*32+13*14
struct.pack_into('<I',b,12,payload)
b[20:20+payload]=b'\x00'*payload
struct.pack_into('<I',b,16,zlib.crc32(b[:16]+b[20:20+payload]))
b[-4:]=b'B2OK'
s=Snapshot(1,tuple((None,None) for _ in range(12)),tuple(None for _ in range(13)))
assert bytes(b)==_snapshot_bytes(s)
print('PASS C bootstrap superblock parity with V2 host format')
