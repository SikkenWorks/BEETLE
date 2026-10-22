#!/usr/bin/env python3
"""BEETLE V1.4.16 - OFFLINE, READ-ONLY legacy-to-Storage-V2 migration preflight.

Accepts a manifest explicitly accounting for all twelve legacy slots and optional
U01..U13 compressed BD4S files. It NEVER opens a device, flashes firmware,
or produces a device-writeable image. Authenticates .beetle exports, validates
TRK1 and BD4S integrity, simulates fresh Storage V2 allocation, and verifies
all data survives an in-memory power-cycle byte-for-byte.

This does NOT prove hardware flash ownership or migration-in-place safety.
"""
from __future__ import annotations
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/storage_v2'))
sys.path.insert(0,str(ROOT/'tools/dpcm4'))
from storage_v2_sim import Storage, Nor, ProjectCodec, NotEnoughSafeSpace, CorruptData
import beetle_dpcm4 as bd4

# Shared/public distribution key used by the BEETLE V2 Project Manager.
KEY=bytes.fromhex('7112d23abe7c813667a301df4e9470c82d519f09d582134abf7268ec04ca3df9')

class MigrationError(ValueError):pass

def read_project(path:Path)->bytes:
    raw=path.read_bytes()
    if raw.startswith(b'BEETLE01\x02'):
        try:
            from cryptography.hazmat.primitives.ciphers.aead import AESGCM
            if len(raw)<38:raise MigrationError('truncated V2 .beetle backup')
            raw=AESGCM(KEY).decrypt(raw[9:21],raw[21:],raw[:21])
        except ImportError as e:
            raise MigrationError('cryptography required for authenticated .beetle backups') from e
        except Exception as e:
            raise MigrationError('BEETLE AES-GCM authentication failed') from e
    if len(raw)<24 or raw[:4]!=b'TRK1':raise MigrationError('not an authenticated BEETLE project/TRK1 export')
    version=struct.unpack_from('<H',raw,4)[0]
    stored_size=struct.unpack_from('<I',raw,8)[0]
    if not 1<=version<=24 or stored_size!=len(raw) or len(raw)>12256:
        raise MigrationError(f'unsupported TRK1 schema/size V{version}, {len(raw)} bytes')
    if zlib.crc32(raw[:16])&0xffffffff != struct.unpack_from('<I',raw,16)[0]:
        raise MigrationError('TRK1 header CRC mismatch')
    if zlib.crc32(raw[24:])&0xffffffff != struct.unpack_from('<I',raw,12)[0]:
        raise MigrationError('TRK1 payload CRC mismatch')
    return raw

def read_sample(path:Path)->bytes:
    raw=path.read_bytes()
    try:bd4.decode(raw)
    except Exception as e:raise MigrationError('invalid BD4S source (CRC/frame validation)') from e
    return raw

def load_entries(manifest:Path):
    cfg=json.loads(manifest.read_text(encoding='utf-8'))
    if cfg.get('format')!='BEETLE-MIGRATION-PREFLIGHT-1':
        raise MigrationError('wrong manifest format')
    entries=cfg.get('projects')
    if not isinstance(entries,list) or len(entries)!=12:
        raise MigrationError('explicit record of all twelve project slots required')
    projects={}
    for obj in entries:
        if not isinstance(obj,dict) or type(obj.get('slot')) is not int:raise MigrationError('invalid project entry')
        slot=obj['slot']
        if not 1<=slot<=12 or slot in projects:raise MigrationError('duplicate/out-of-range project slot')
        status=obj.get('status')
        if status=='empty':
            if 'path' in obj:raise MigrationError('empty slot must not carry a filename')
            projects[slot]=None
        elif status=='file':
            name=obj.get('path')
            if not isinstance(name,str) or not name or Path(name).is_absolute():raise MigrationError('project path must be relative')
            path=(manifest.parent/name).resolve()
            if not path.is_relative_to(manifest.parent.resolve()):raise MigrationError('project path escapes backup directory')
            if not path.is_file():raise MigrationError(f'missing project export for slot {slot}')
            projects[slot]=read_project(path)
        else:raise MigrationError('each project requires explicit empty/file status')
    samples={}
    for obj in cfg.get('samples',[]):
        if not isinstance(obj,dict) or type(obj.get('slot')) is not int:raise MigrationError('invalid sample entry')
        slot=obj['slot']
        if not 1<=slot<=13 or slot in samples:raise MigrationError('duplicate/out-of-range sample slot')
        name=obj.get('path')
        if not isinstance(name,str) or not name or Path(name).is_absolute():raise MigrationError('sample path must be relative')
        path=(manifest.parent/name).resolve()
        if not path.is_relative_to(manifest.parent.resolve()) or not path.is_file():
            raise MigrationError('sample filename invalid or unavailable')
        samples[slot]=read_sample(path)
    return projects,samples

def simulate(manifest:Path)->dict:
    projects,samples=load_entries(manifest)
    source=ROOT/'firmware/fm1-native/beetle_project_pack_v2.c'
    with tempfile.TemporaryDirectory(prefix='beetle-migration-') as tmp:
        so=Path(tmp)/'p.so'
        subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fPIC','-shared',str(source),'-o',str(so)],
                       check=True,capture_output=True)
        codec=ProjectCodec(ctypes.CDLL(str(so)))
        nor=Nor(authorized=True)  # isolated memory model; NOT authorization for actual FM1
        store=Storage(nor,codec)
        for slot,raw in projects.items():
            if raw is not None:store.save_project(slot-1,raw)
        for slot,raw in samples.items():store.replace_sample(slot-1,raw)
        restarted=Storage(Nor(bytes(nor.bytes),authorized=True),codec)
        for slot,raw in projects.items():
            if restarted.load_project(slot-1)!=raw:raise MigrationError(f'project {slot} failed reboot readback')
        for slot,raw in samples.items():
            if restarted.load_sample(slot-1)!=raw:raise MigrationError(f'sample U{slot:02} failed reboot readback')
        stats=restarted.stats()
        if stats['erased_sectors']<4:raise MigrationError('4-sector reserve violation')
        return {'status':'OFFLINE_PREFLIGHT_ONLY','fm1_flash_written':False,
                'source_slots_verified':12,'project_files':sum(x is not None for x in projects.values()),
                'empty_slots':sum(x is None for x in projects.values()),
                'samples':len(samples),'sample_total_seconds':round(sum(struct.unpack_from('<I',x,8)[0] for x in samples.values())/22050,3),
                'project_hashes':{str(i):hashlib.sha256(x).hexdigest() for i,x in projects.items() if x is not None},
                'sample_hashes':{f'U{i:02}':hashlib.sha256(x).hexdigest() for i,x in samples.items()},
                'verified_after_simulated_reboot':True,'free_erased_sectors':stats['erased_sectors'],
                'minimum_reserve_sectors':4,
                'WARNING':'DOES NOT AUTHORIZE FLASHING. No on-device migration, loader recovery or DMA tests.'}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest',type=Path,help='Explicit twelve-slot backup manifest JSON')
    parser.add_argument('--report',type=Path,help='Write only a diagnostic JSON report, never a flash image')
    args=parser.parse_args()
    try: result=simulate(args.manifest.resolve())
    except (MigrationError,NotEnoughSafeSpace,CorruptData,FileNotFoundError,ValueError,subprocess.CalledProcessError) as e:
        print(f'PREFLIGHT REJECTED: {e}',file=sys.stderr)
        return 1
    text=json.dumps(result,indent=2,ensure_ascii=False)+'\n'
    print(text,end='')
    if args.report:args.report.write_text(text,encoding='utf-8')
    return 0

if __name__=='__main__':sys.exit(main())
