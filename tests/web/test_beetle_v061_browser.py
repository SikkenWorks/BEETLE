from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[2]
html=(root/'web/BEETLE-V0.61-WebSimulator.html').read_text()
assert 'HELP: K1 SCROLLS ONLY' in html
assert 'K4 HEADINGS' not in html
html=html.replace('const audio=new TrackAudio();', """window.__v061={screen:()=>model.screen,help:()=>model.helpOffset,bpm:()=>model.project.bpm,len:()=>pattern(model.project,model.project.selectedPattern,true).length,route,rotate,hold:()=>{seqHeld=true;seqChordUsed=false;},release:()=>{seqHeld=false;seqChordUsed=false;}};"""+ 'const audio=new TrackAudio();',1)
with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(viewport={'width':1120,'height':780})
 errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(html,wait_until='load',timeout=60000)
 page.wait_for_timeout(250)
 # Physical SEQ+K4 moves length, never BPM; plain K4 retains BPM control
 initial=page.evaluate('window.__v061.bpm()')
 page.evaluate('window.__v061.hold()')
 page.evaluate('window.__v061.rotate("k4",-3)')
 assert page.evaluate('window.__v061.len()')==13
 assert page.evaluate('window.__v061.bpm()')==initial
 page.evaluate('window.__v061.rotate("k4",999)')
 assert page.evaluate('window.__v061.len()')==16
 page.evaluate('window.__v061.release()')
 page.evaluate('window.__v061.rotate("k4",1)')
 assert page.evaluate('window.__v061.bpm()')==initial+1
 page.evaluate('window.__v061.route("HOME")')
 page.evaluate('window.__v061.rotate("k1",4)')
 page.evaluate('window.__v061.route("SEL")')
 assert page.evaluate('window.__v061.screen()')=='help'
 before=page.evaluate('window.__v061.help()');bpm=page.evaluate('window.__v061.bpm()')
 page.evaluate('window.__v061.rotate("k4",6)')
 assert page.evaluate('window.__v061.help()')==before
 assert page.evaluate('window.__v061.bpm()')==bpm
 page.evaluate('window.__v061.rotate("k1",4)')
 assert page.evaluate('window.__v061.help()')==before+4
 assert not errors,errors
 print('PASS BEETLE V0.61 browser: HELP K1 only, SEQ+K4 edits length, never BPM')
 browser.close()
