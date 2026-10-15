#!/usr/bin/env python3
"""V0.58: verify exact simulator sprites, moon continuity and boot-loop conditions."""
import re,base64,io
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parents[2]
html=(root.parent/(root.parent.name+'-WebSimulator.html')).read_text()
m=re.search(r"const bootFrames=\['data:image/png;base64,([^']+)','data:image/png;base64,([^']+)'\]",html)
assert m,'two full-resolution embedded frames required'
ref=Image.open(root/'assets/loading-screen/LoadingScreen-background.png').convert('RGB')
assert ref.size==(240,240)
assert ref.getpixel((1,1))!=(0,0,0),'moon visible at upper-left'
frames=[]
for idx in range(2):
 f=Image.open(io.BytesIO(base64.b64decode(m.group(idx+1)))).convert('RGB')
 assert f.size==(240,240)
 ref_image=Image.open(root/'assets/loading-screen'/f'BEETLE-V0.58-LoadingScreen-Frame{idx+1}.png').convert('RGB')
 assert list(f.getdata())==list(ref_image.getdata()),'websim frame mismatch'
 frames.append(f)
 count=0
 for y in range(240):
  for x in range(240):
   if f.getpixel((x,y))==(255,0,240):count+=1
   else:assert f.getpixel((x,y))==ref.getpixel((x,y)), 'moon/tree shifted'
 print('PASS: simulator frame',idx,'unchanged beetle pixels',count)
assert frames[0].getpixel((1,1))==frames[1].getpixel((1,1))
assert frames[0].tobytes()!=frames[1].tobytes()
assert 'bootTicker=setInterval' in html and '},100)' in html
assert "$('#splash').classList.contains('hidden')" in html
native=(root/'firmware/fm1-native/bringup.c').read_text()
assert 'beetle_boot_frame^=1u' in native
assert 'beetle_boot_next_ms=etch_ms+100u' in native
assert 'beetle_boot_active=0u' in native
print('PASS: animation alternates at 100 ms while splash stays active')
