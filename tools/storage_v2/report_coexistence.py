#!/usr/bin/env python3
"""Read-only sector report for a local BEETLE Storage V2 image.

Accepts a 288KiB partition image or a private 1MiB full-flash dump. Does not
connect to the FM1, modify the image, or export sensitive project contents.
Usage: python tools/storage_v2/report_coexistence.py /path/to/image.bin
"""
import argparse
import ctypes
import json
import subprocess
import tempfile
from pathlib import Path

from storage_v2_sim import Nor,Storage,ProjectCodec,UnsafeLayout,SECTOR,SECTORS
ROOT=Path(__file__).resolve().parents[2]


def report(path:Path)->dict:
    data=path.read_bytes()
    if len(data)==1048576:
        data=data[0x97000:0xDF000]
    elif len(data)!=SECTOR*SECTORS:
        raise ValueError('Expected 288 KiB V2 partition or full 1 MiB FM1 image')
    with tempfile.TemporaryDirectory(prefix='beetle-v2-inspect-') as t:
        library=Path(t)/'codec.so'
        subprocess.run(['cc','-O2','-std=c11','-fPIC','-shared',
            str(ROOT/'firmware/fm1-native/beetle_project_pack_v2.c'),
            '-o',str(library)],check=True,capture_output=True)
        codec=ProjectCodec(ctypes.CDLL(str(library)))
        store=Storage(Nor(data,authorized=False),codec)
        if not store.available:raise UnsafeLayout('no committed V2 index: legacy flash cannot be interpreted as V2')
        stats=store.stats()
        if stats['shared_legacy_sectors']:
            raise UnsafeLayout('mixed project/sample sectors found: V2 coexistence audit failed')
        lengths=[len(store.load_project(i)) if store.load_project(i) is not None else 0 for i in range(12)]
        samples=[len(store.load_sample(i)) if store.load_sample(i) is not None else 0 for i in range(13)]
        return {'format':'BEETLE-V2-COEXISTENCE-REPORT',
                'flash_bytes':stats['total_bytes'],
                'project_slots':12,'sample_slots':13,
                'project_bytes_uncompressed':sum(lengths),
                'sample_bytes_compressed':sum(samples),
                'individual_project_bytes':lengths,
                'individual_sample_bytes':samples,
                'projects_in_use':sum(bool(x) for x in lengths),
                'samples_in_use':sum(bool(x) for x in samples),
                'project_sectors_used':stats['project_sectors'],
                'sample_sectors_used':stats['sample_sectors'],
                'erased_unallocated_sectors':stats['erased_sectors'],
                'minimum_safety_sector_reserve':stats['guard_erased_sectors'],
                'dirty_unreferenced_sectors':stats['unreferenced_dirty_sectors'],
                'sector_overlap':False,
                'note':'This is a read-only report, not permission to flash or a guarantee of future capacity.'}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('image',type=Path)
    args=ap.parse_args()
    try:print(json.dumps(report(args.image),indent=2))
    except (OSError,ValueError,UnsafeLayout,subprocess.CalledProcessError) as exc:
        ap.exit(1,f'BEETLE V2 READ-ONLY AUDIT FAILED: {exc}\n')

if __name__=='__main__':main()
