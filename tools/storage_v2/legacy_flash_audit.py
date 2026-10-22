#!/usr/bin/env python3
"""BEETLE offline legacy A/B journal auditor and optional project exporter.

Accepts a *local* 1,048,576-byte FM1 SPI-NOR image; never opens a USB device,
modifies flash, constructs a flashable image or infers erased sectors as owned.

All 24 banks and the complete ownership-marker sector are authenticated before
ANY exports are created. --export-dir writes raw verified TRK1 projects plus an
explicit twelve-slot manifest for migration_preflight.py. Keep full dumps and
project exports private. No hardware flash ownership/recovery is proven here.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

FLASH_BYTES = 0x100000
BASE, END, MARKER, MARKER_END = 0x97000, 0xDF000, 0xDF000, 0xE0000
BANK_BYTES, HEAD_BYTES, SLOTS, BANKS = 12288, 32, 12, 2
MAX_TRK1_VERSION = 24
MAGIC = b'TRKJ'
MARKER_MAGIC = b'BTL33FS1'

class AuditError(ValueError):
    pass

def u16(b: bytes, at: int) -> int:
    return struct.unpack_from('<H', b, at)[0]

def u32(b: bytes, at: int) -> int:
    return struct.unpack_from('<I', b, at)[0]

def crc(b: bytes) -> int:
    return zlib.crc32(b) & 0xFFFFFFFF

def expected_marker() -> bytes:
    header = (MARKER_MAGIC + struct.pack('<IIII', BASE, END, BANK_BYTES, SLOTS))
    return header + struct.pack('<I', crc(header)) + b'\xff' * 4

def scan_bank(img: bytes, slot: int, bank: int) -> dict:
    begin = BASE + (slot * BANKS + bank) * BANK_BYTES
    raw = img[begin:begin + BANK_BYTES]
    header = raw[:HEAD_BYTES]
    info = {'slot': slot + 1, 'bank': 'AB'[bank], 'address': f'0x{begin:05X}'}
    if header == b'\xff' * HEAD_BYTES:
        if raw != b'\xff' * BANK_BYTES:
            raise AuditError(f'slot {slot+1} bank {"AB"[bank]}: erased header but non-erased tail (foreign or interrupted data)')
        return {**info, 'state': 'empty'}
    if (header[:4] != MAGIC or u16(header, 4) != 1 or header[6] != slot
        or header[7] != (slot ^ 255) or header[20] not in (0, 255)):
        raise AuditError(f'slot {slot+1} bank {"AB"[bank]}: foreign/unknown header')
    gen, n, data_crc = u32(header, 8), u32(header, 12), u32(header, 16)
    if n < 42 or n > BANK_BYTES - HEAD_BYTES:
        raise AuditError(f'slot {slot+1} bank {"AB"[bank]}: invalid payload length')
    if any(b != 255 for b in raw[HEAD_BYTES + n:]):
        raise AuditError(f'slot {slot+1} bank {"AB"[bank]}: unexpected bytes after project (foreign tail)')
    payload = raw[HEAD_BYTES:HEAD_BYTES + n]
    valid = (header[20] == 0 and u32(header, 24) == crc(header[:20])
             and crc(payload) == data_crc and payload[:4] == b'TRK1'
             and 1 <= u16(payload, 4) <= MAX_TRK1_VERSION
             and u16(payload, 6) == slot and u32(payload, 8) == n
             and u32(payload, 12) == crc(payload[24:])
             and u32(payload, 16) == crc(payload[:16]))
    # A torn write is recorded as a damaged owned generation, not trusted.
    return {**info, 'state': 'committed' if valid else 'incomplete_or_corrupt',
            'generation': gen, 'bytes': n,
            **({'schema': u16(payload, 4), 'sha256': hashlib.sha256(payload).hexdigest(),
                '_payload': payload} if valid else {})}

def is_newer(a: int, b: int) -> bool:
    # Mirrors native signed 32-bit generation comparison.
    delta = (a - b) & 0xffffffff
    return 0 < delta < 0x80000000

def audit_image(raw: bytes) -> tuple[dict, dict[int, bytes]]:
    if len(raw) != FLASH_BYTES:
        raise AuditError(f'expected exact 1,048,576-byte full flash dump; got {len(raw)}')
    if raw[MARKER:MARKER + 32] != expected_marker():
        raise AuditError('ownership marker missing/invalid: refusing to assume flash is BEETLE-owned')
    if raw[MARKER + 32:MARKER_END] != b'\xff' * (MARKER_END - MARKER - 32):
        raise AuditError('ownership marker sector has unexpected data')
    slots, files = [], {}
    for slot in range(SLOTS):
        banks = [scan_bank(raw, slot, b) for b in range(BANKS)]
        committed = [b for b in banks if b['state'] == 'committed']
        if not committed:
            if any(b['state'] != 'empty' for b in banks):
                raise AuditError(f'slot {slot+1}: no recoverable committed generation')
            slots.append({'slot': slot + 1, 'status': 'empty', 'banks': banks})
            continue
        selected = committed[0]
        if len(committed) == 2:
            second = committed[1]
            if selected['generation'] == second['generation']:
                if selected['_payload'] != second['_payload']:
                    raise AuditError(f'slot {slot+1}: conflicting equal generations')
            elif is_newer(second['generation'], selected['generation']):
                selected = second
            elif not is_newer(selected['generation'], second['generation']):
                raise AuditError(f'slot {slot+1}: ambiguous generation wraparound')
        files[slot + 1] = selected['_payload']
        slots.append({'slot': slot + 1, 'status': 'file',
                      'chosen_bank': selected['bank'], 'generation': selected['generation'],
                      'schema': selected['schema'], 'bytes': selected['bytes'],
                      'sha256': selected['sha256'],
                      'damaged_sister_bank': any(b['state']=='incomplete_or_corrupt' for b in banks),
                      'banks': banks})
    # Never leak source TRK1 payloads into JSON.
    for s in slots:
        for bank in s['banks']:
            bank.pop('_payload', None)
    report = {'format': 'BEETLE-LEGACY-FLASH-AUDIT-1',
              'operation': 'OFFLINE_READ_ONLY', 'flash_modified': False,
              'image_sha256': hashlib.sha256(raw).hexdigest(),
              'full_flash_bytes': FLASH_BYTES, 'marker': 'valid (entire 4KiB marker sector)',
              'journal_range': [f'0x{BASE:05X}', f'0x{END-1:05X}'],
              'projects_recovered': len(files), 'empty_slots': SLOTS-len(files),
              'slots': slots,
              'limitations': ['Source of dump and hardware flash ownership not independently verified',
                              'No FM1 write path, no OTA/recovery proof, no on-device Storage V2 migration',
                              'Keep flash dumps and project exports private']}
    return report, files

def export_projects(destination: Path, raw: bytes) -> dict:
    report, files = audit_image(raw)  # Validate ALL banks before touching export directory.
    if destination.exists():
        raise AuditError('export destination must not exist (avoid overwriting prior backups)')
    if not destination.parent.is_dir():
        raise AuditError('export parent directory does not exist')
    manifest = {'format': 'BEETLE-MIGRATION-PREFLIGHT-1', 'projects': [], 'samples': []}
    # No silent creation/replacement of existing backups.
    try:
        destination.mkdir(mode=0o700)
        for slot in range(1, SLOTS + 1):
            if slot in files:
                name = f'SLOT-{slot:02}-verified.trk'
                (destination / name).write_bytes(files[slot])
                manifest['projects'].append({'slot': slot, 'status': 'file', 'path': name})
            else:
                manifest['projects'].append({'slot': slot, 'status': 'empty'})
        (destination / 'MIGRATION-MANIFEST.json').write_text(json.dumps(manifest, indent=2) + '\n')
        (destination / 'LEGACY-AUDIT.json').write_text(json.dumps(report, indent=2) + '\n')
    except Exception:
        # Roll back only the files this exporter wrote; never touch the source dump.
        for file in destination.glob('*'):
            if file.is_file(): file.unlink()
        destination.rmdir()
        raise
    return report

def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('image', type=Path, help='local 1MiB read-only SPI NOR image')
    p.add_argument('--export-dir', type=Path, help='NEW private directory for 12-slot TRK1 exports')
    p.add_argument('--report', type=Path, help='optional read-only JSON report')
    args = p.parse_args()
    try:
        # Check all output targets before exporting anything. Never replace an existing file.
        if args.report and args.report.resolve() == args.image.resolve():
            raise AuditError('report must not overwrite the source flash image')
        if args.report and args.report.exists():
            raise AuditError('report destination must not exist')
        if args.export_dir and args.report and args.report.resolve().is_relative_to(args.export_dir.resolve()):
            raise AuditError('report must be outside the new export directory')
        raw = args.image.read_bytes()
        report = export_projects(args.export_dir, raw) if args.export_dir else audit_image(raw)[0]
        output = json.dumps(report, indent=2) + '\n'
        if args.report:
            with args.report.open('x', encoding='utf-8') as f:
                f.write(output)
        print(output)
    except (AuditError, OSError) as exc:
        print(f'READ-ONLY AUDIT REFUSED: {exc}', file=sys.stderr)
        return 1
    return 0

if __name__ == '__main__':
    sys.exit(main())
