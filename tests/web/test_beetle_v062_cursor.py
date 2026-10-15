"""V0.62 browser regression: pattern length is the only edit-step navigation bound."""
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[2]
html=(root/'web/BEETLE-V0.62-WebSimulator.html').read_text()
needle='const audio=new TrackAudio();'
assert needle in html
html=html.replace(needle,"""window.__v062={model,route,rotate,editShortcut,doBlock,pattern,historyMove,editTools,screen:()=>model.screen,hold:()=>{seqHeld=true;},release:()=>{seqHeld=false;}};"""+needle,1)
with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True, executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(viewport={'width':1120,'height':780}); errs=[]
 page.on('pageerror',lambda e:errs.append(str(e)))
 page.set_content(html,wait_until='load',timeout=60000)
 page.wait_for_timeout(250)
 # Changing length clamps row and does not alter BPM.
 page.evaluate('window.__v062.model.row=15')
 bpm=page.evaluate('window.__v062.model.project.bpm')
 page.evaluate('window.__v062.hold();window.__v062.rotate("k4",-8);window.__v062.release()')
 assert page.evaluate('window.__v062.pattern(window.__v062.model.project,window.__v062.model.project.selectedPattern,false).length')==8
 assert page.evaluate('window.__v062.model.row')==7
 assert page.evaluate('window.__v062.model.project.bpm')==bpm
 page.evaluate('window.__v062.rotate("k1",100)')
 assert page.evaluate('window.__v062.model.row')==7
 page.evaluate('window.__v062.rotate("k1",-100)')
 assert page.evaluate('window.__v062.model.row')==0
 # Dynamics cursor obeys same bounds.
 page.evaluate('window.__v062.editShortcut("dynamics");window.__v062.rotate("k1",100)')
 assert page.evaluate('window.__v062.model.row')==7
 page.evaluate('window.__v062.route("SEQ");window.__v062.editShortcut("block");window.__v062.rotate("k4",100);window.__v062.rotate("k1",100)')
 assert page.evaluate('[window.__v062.editTools.first,window.__v062.editTools.last]')==[7,7]
 # Longer pattern exposes retained data, shortened pattern hides it, redo clamps.
 page.evaluate('window.__v062.route("SEQ");window.__v062.hold();window.__v062.rotate("k4",8);window.__v062.release()')
 assert page.evaluate('window.__v062.pattern(window.__v062.model.project,window.__v062.model.project.selectedPattern,false).length')==16
 page.evaluate('window.__v062.model.row=15;window.__v062.hold();window.__v062.rotate("k4",-13);window.__v062.release()')
 assert page.evaluate('window.__v062.model.row')==2
 page.evaluate('window.__v062.editShortcut("undo")')
 assert page.evaluate('window.__v062.pattern(window.__v062.model.project,window.__v062.model.project.selectedPattern,false).length')==16
 page.evaluate('window.__v062.model.row=15;window.__v062.editShortcut("redo")')
 assert page.evaluate('window.__v062.model.row')==2
 assert not errs,errs
 print('PASS V0.62 WebSimulator: short patterns clamp K1 in tracker/dynamics/block; SEQ+K4, undo/redo, BPM')
 browser.close()
