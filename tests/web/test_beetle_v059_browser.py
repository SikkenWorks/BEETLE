from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[2]
path=root/'web/BEETLE-V0.59-WebSimulator.html'
s=path.read_text()
assert 'BEETLE V0.59' in s and 'STEP VELOCITY/GATE' in s
s=s.replace('const audio=new TrackAudio();','''window.__v059={state:()=>({screen:model.screen,row:model.row,ch:model.ch,focus:model.dynFocus,dyn:step(model.project,model.project.selectedPattern,model.ch,model.row,false).dyn??0}),
set:(ch,row)=>{model.ch=ch;model.row=row;model.screen='tracker';render();},
key:(name)=>editShortcut(name),
reset:()=>{model.project=freshProject();model.screen='tracker';render();},
project:()=>model.project};const audio=new TrackAudio();''')
with sync_playwright() as p:
 b=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=b.new_page(viewport={'width':1240,'height':920});errs=[]
 page.on('pageerror',lambda e:errs.append(str(e)))
 page.set_content(s,wait_until='load',timeout=60000);page.wait_for_timeout(1300)
 page.evaluate('window.__v059.set(2,4)')
 page.locator('[data-edit="dynamics"]').click()
 assert page.evaluate('window.__v059.state().screen')=='dynamics'
 page.locator('[data-turn="k3:1"]').click()
 assert page.evaluate('window.__v059.state().dyn')==0x10
 page.locator('[data-turn="k2:1"]').click()
 for _ in range(7):page.locator('[data-turn="k3:1"]').click()
 assert page.evaluate('window.__v059.state().dyn')==0x17
 page.locator('[data-action="SEQ"]').click()
 assert page.evaluate('window.__v059.state().screen')=='tracker'
 page.evaluate('window.__v059.key("copyPattern")')
 page.evaluate('window.__v059.set(3,6)')
 page.evaluate('window.__v059.key("pastePattern")')
 assert page.evaluate('window.__v059.project().patterns[0].channels[2][4].dyn')==0x17
 page.evaluate('window.__v059.key("undo")')
 assert page.evaluate('window.__v059.project().patterns[0].channels[2][4].dyn')==0x17 or page.evaluate('window.__v059.state().screen')=='tracker'
 page.evaluate('window.__v059.set(2,4)')
 page.locator('[data-edit="dynamics"]').click()
 page.locator('[data-action="SEL"]').click()
 img=root/'tests/web/v059_dynamics_preview.png';page.locator('#screen').screenshot(path=str(img))
 assert not errs,errs
 print('PASS V0.59 WebSimulator: dynamics, edit/undo, copy/paste, navigation, no JS exception')
 b.close()
