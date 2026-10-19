"""V19 simulator native codec binary roundtrip (no physical device)."""
from pathlib import Path
from playwright.sync_api import sync_playwright
s=(Path(__file__).resolve().parents[2]/'simulator.html').read_text()
a='})(TRACK_CORE,TRACK_AUDIO);</script>'
s=s.replace(a,'window.__testSong={model,songNewRow,songLanes};\n'+a)
with sync_playwright() as pw:
 b=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 page=b.new_page();errs=[];page.on('pageerror',lambda e:errs.append(str(e)))
 page.set_content(s);page.wait_for_timeout(900)
 o=page.evaluate('''() => {
  const {model:m,songNewRow}=window.__testSong, native=window.__beetleBackupTest.BeetleNative;
  const row=songNewRow(1);row.lanes=[2,2,2,2,2,0];m.project.song.push(row);
  let encoded=native.encode(m.project);
  if(encoded[4]!==19)throw Error('Schema version is not 19');
  const decoded=native.decode(encoded);
  if(decoded.song.length!==2||decoded.song[1].lanes[5]!==0||decoded.song[1].lanes[0]!==2)throw Error('V19 lanes lost');
  return {size:encoded.length, schema:encoded[4],lanes:decoded.song[1].lanes};
 }''')
 assert not errs, errs
 print('PASS browser native V19 export/import',o)
 b.close()
