#!/usr/bin/env python3
"""Local/offline BEETLE V2 backup decoder for diagnostic tests. GPL-3.0-only.

The distribution key is already public in BEETLE Project Manager; it is not a
secret. Verify AES-GCM tag BEFORE exporting raw bytes. V1 password-based backups
are deliberately not supported by this diagnostic script. Never upload dumps.
"""
import argparse
import struct
import zlib
from pathlib import Path
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
KEY = bytes.fromhex('7112d23abe7c813667a301df4e9470c82d519f09d582134abf7268ec04ca3df9')
p=argparse.ArgumentParser();p.add_argument('input',type=Path);p.add_argument('output',type=Path)
a=p.parse_args()
s=a.input.read_bytes()
if len(s)<38 or s[:9]!=b'BEETLE01\x02':raise SystemExit('Not a BEETLE V2 backup')
raw=AESGCM(KEY).decrypt(s[9:21],s[21:],s[:21])
if len(raw)<42 or raw[:4]!=b'TRK1' or len(raw)!=struct.unpack_from('<I',raw,8)[0] or zlib.crc32(raw[:16])!=struct.unpack_from('<I',raw,16)[0] or zlib.crc32(raw[24:])!=struct.unpack_from('<I',raw,12)[0]:raise SystemExit('Invalid project CRC or internal length')
a.output.write_bytes(raw)
print(f'CRC verified: schema V{struct.unpack_from("<H",raw,4)[0]}, {len(raw)} bytes; raw debug file written locally (do not publish)')
