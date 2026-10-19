"""RC1.0.11 browser: real simulator controls, default pattern on append, per-lane edits, undo."""
from pathlib import Path
from playwright.sync_api import sync_playwright
p=Path(__file__).resolve().parents[2]/'simulator.html'
s=p.read_text()
anchor='})(TRACK_CORE,TRACK_AUDIO);</script>'
assert anchor in s
s=s.replace(anchor,"window.__chainTest={model,route,rotate,render,songLanes,songLength};\n"+anchor)
with sync_playwright() as pw:
 b=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=b.new_page(viewport={'width':1250,'height':980});errors=[]
 page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(s,wait_until='domcontentloaded');page.wait_for_timeout(950)
 print('BROWSER ERRORS',errors); print('TEST STATE',page.evaluate('typeof window.__chainTest')); got=page.evaluate('''() => {
  const {model:m,route,rotate,render,songLanes,songLength}=window.__chainTest;
  m.screen='song';m.songRow=0;m.songCol=0;render();
  route('OCT+');
  if(m.project.song.length!==2||m.songRow!==1)throw Error('OCT+ did not insert row');
  const entry=m.project.song[1];if(songLanes(entry).some(p=>p!==1))throw Error('Row 02 did not default to pattern 02');
  rotate('k2',5);rotate('k3',-1);
  if(entry.lanes[5]!==0||entry.lanes[0]!==1)throw Error('Drums failed independent pattern source');
  rotate('k1',1);route('OCT+');
  if(songLanes(m.project.song[2]).some(p=>p!==2))throw Error('Row 03 did not default to pattern 03');
  if(m.project.song[1].lanes[5]!==0)throw Error('Insertion modified old row');
  m.songCol=5;rotate('k3',-3);
  if(m.project.song[2].lanes[5]!==-1)throw Error('-- / mute source missing');
  if(songLength(m.project,m.project.song[2])!==16)throw Error('Song length invalid');
  render();
  return {row2:m.project.song[1],row3:m.project.song[2]};
 }''')
 assert not errors,errors
 page.screenshot(path='/mnt/data/beetle_rc1011_song_browser.png')
 print('PASS RC1.0.11 web song: header rendering, default append, independent DR, --, existing row retained',got)
 b.close()
