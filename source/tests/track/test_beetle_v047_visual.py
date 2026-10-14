#!/usr/bin/env python3
"""Compare rendered native framebuffer with original GIF in RGB565 at EVERY pixel."""
from PIL import Image
from pathlib import Path
import struct,subprocess
root=Path(__file__).resolve().parents[2]
exe=Path('/tmp/beetle-v047-visual')
out=Path('/tmp/beetle-v047-visual.rgb565')
subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror', '-I'+str(root/'firmware/fm1-native'),str(root/'tests/track/test_beetle_v047_visual.c'),'-o',str(exe)],check=True)
subprocess.run([str(exe),str(out)],check=True)
data=out.read_bytes();assert len(data)==240*240*2*2
im=Image.open(root/'assets/loading-screen/LoadingScreen.gif');assert im.n_frames==2
for f in range(2):
 im.seek(f)
 assert im.info['duration']==100
 rgb=im.convert('RGB')
 got=struct.unpack_from('<%dH'%(240*240),data,f*240*240*2)
 expected=[((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in list(rgb.getdata())]
 diffs=[i for i in range(240*240) if expected[i]!=got[i]]
 assert not diffs, f'Frame {f} differs at {len(diffs)} pixels; first={diffs[:10]}'
 print(f'PASS: 100ms frame {f}, all 57,600 pixels match original GIF in RGB565')
