#!/usr/bin/env python3
"""Compare rendered native framebuffer with user moon/tree + unchanged sprite masks."""
from PIL import Image
from pathlib import Path
import struct,subprocess
root=Path(__file__).resolve().parents[2]
exe=Path('/tmp/beetle-v047-visual')
out=Path('/tmp/beetle-v047-visual.rgb565')
subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror', '-I'+str(root/'firmware/fm1-native'),str(root/'tests/track/test_beetle_v047_visual.c'),'-o',str(exe)],check=True)
subprocess.run([str(exe),str(out)],check=True)
data=out.read_bytes();assert len(data)==240*240*2*2
from PIL import Image
bg=Image.open(root/'assets/loading-screen/LoadingScreen-background.png').convert('RGB')
original=Image.open(root/'assets/loading-screen/LoadingScreen.gif')
assert original.n_frames==2 and original.size==(240,240)
for f in range(2):
 original.seek(f)
 assert original.info['duration']==100
 sprite=Image.open(root/'assets/loading-screen'/f'LoadingScreen-beetle{2-f}.png').convert('RGB')
 old=original.convert('RGB')
 expected=[]
 got=struct.unpack_from('<%dH'%(240*240),data,f*240*240*2)
 for y in range(240):
  for x in range(240):
   is_pink=sprite.getpixel((x,y))==(255,0,240)
   assert (old.getpixel((x,y))==(255,0,240))==is_pink, (f,x,y)
   r,g,b=(255,0,240) if is_pink else bg.getpixel((x,y))
   expected.append((r>>3)<<11|(g>>2)<<5|(b>>3))
 diffs=[i for i in range(240*240) if expected[i]!=got[i]]
 assert not diffs, f'Frame {f} differs at {len(diffs)} pixels; first={diffs[:10]}'
 print(f'PASS: V0.58 100ms frame {f}, all 57,600 pixels equal new static moon/tree plus original beetle (RGB565)')
