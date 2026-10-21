from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[2]
html=(root/'web/BEETLE-V0.61-WebSimulator.html').read_text()
assert 'HELP: K1 SCROLLS ONLY' in html and 'FM1 DIGITAL DRUMS' not in html
html=html.replace('const audio=new TrackAudio();', '''window.__v060={
 state:()=>({screen:model.screen,help:model.helpOffset,row:model.row,pattern:model.project.selectedPattern,queued:model.queuedPattern,dirty:model.dirty}),
 route:(action)=>route(action),rotate:(knob,delta)=>rotate(knob,delta),
 reset:()=>{model.project=freshProject();model.screen='tracker';model.row=0;model.ch=0;model.col=1;model.dirty=false;render();},
 seed:()=>{const s=step(model.project,0,0,0,true);s.note=62;model.project.name='V060';},
 getnote:()=>step(model.project,0,0,0,false).note,
 save:()=>store(0),load:()=>loadSlot(0),
 playFake:()=>{model.playing=true;model.songMode=false;audio.currentPattern=model.project.selectedPattern;audio.pendingPattern=null;},
 stopFake:()=>{model.playing=false;audio.pendingPattern=null;model.queuedPattern=null;},
 pending:()=>audio.pendingPattern,
 stepLength:()=>pattern(model.project,model.project.selectedPattern,true).length,
 helpLine:(index)=>HELP_LINES[index]};
 const audio=new TrackAudio();''',1)
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(viewport={'width':1200,'height':900})
 errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(html,wait_until='load',timeout=60000);page.wait_for_timeout(800)
 page.evaluate('window.__v060.route("HOME")')
 page.evaluate('window.__v060.rotate("k1",4)')
 page.evaluate('window.__v060.route("SEL")')
 assert page.evaluate('window.__v060.state().screen')=='help'
 start=page.evaluate('window.__v060.state().help')
 page.evaluate('window.__v060.rotate("k4",2)')
 assert page.evaluate('window.__v060.state().help')==start, 'K4 must not scroll HELP'
 page.evaluate('window.__v060.rotate("k1",1)')
 assert page.evaluate('window.__v060.state().help')==start+1
 page.evaluate('window.__v060.reset()')
 page.evaluate('window.__v060.seed()')
 page.evaluate('window.__v060.save()')
 page.evaluate('window.__v060.load()')
 assert page.evaluate('window.__v060.getnote()')==62
 page.evaluate('window.__v060.route("SEL")')
 assert page.evaluate('window.__v060.getnote()')==62, 'Buffered SELECT deleted a newly loaded step'
 page.wait_for_timeout(500)
 page.evaluate('window.__v060.route("SEL")')
 assert page.evaluate('window.__v060.getnote()') is None, 'SELECT must work on a subsequent explicit press'
 page.evaluate('window.__v060.reset()')
 page.locator('[data-edit="lengthMinus"]').click()
 assert page.evaluate('window.__v060.stepLength()')==15
 page.locator('[data-edit="lengthPlus"]').click()
 assert page.evaluate('window.__v060.stepLength()')==16
 page.evaluate('window.__v060.playFake()')
 page.evaluate('window.__v060.rotate("algorithm",1)')
 assert page.evaluate('window.__v060.pending()')==1
 assert page.evaluate('window.__v060.state().pattern')==0,'must not change pattern before quantized boundary'
 assert not errors,errors
 print('PASS BEETLE V0.60 browser: HELP K4 headings, load stale SEL, 1-16 steps, queued pattern, no JS errors')
 browser.close()
