#!/usr/bin/env python3
"""Generate 13 genuinely encoded BD4S fixtures and exercise the native C stage.
No FM1 connected. No firmware or project flash modified.
"""
from pathlib import Path
import math, struct, subprocess, tempfile, sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/dpcm4'))
import beetle_dpcm4 as codec
SRC=ROOT/'firmware/fm1-native'
with tempfile.TemporaryDirectory() as td:
    td=Path(td)
    binary=td/'prefetch'
    cmd=['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=undefined','-fno-sanitize-recover=all','-I',str(SRC),str(SRC/'beetle_bd4_prefetch.c'),str(ROOT/'tests/dpcm4/test_prefetch_stream.c'),'-o',str(binary)]
    subprocess.run(cmd,check=True)
    args=[str(binary)]
    for slot in range(13):
        count=4096+slot*683 if slot<6 else [1,2,63,64,65,257,441000][slot-6]
        pcm=[int(11000*math.sin(2*math.pi*(120+slot*47)*i/22050)*(0.55+0.45*math.cos(i/53))) for i in range(count)]
        encoded=codec.encode(pcm)
        decoded=codec.decode(encoded)
        src=td/f'slot{slot}.bd4s';reference=td/f'slot{slot}.raw'
        src.write_bytes(encoded);reference.write_bytes(struct.pack('<'+'h'*len(decoded),*decoded))
        args.extend([str(src),str(reference)])
    subprocess.run(args,check=True)
