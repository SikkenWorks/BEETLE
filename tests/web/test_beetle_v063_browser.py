from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[2]/'web/BEETLE-V0.63-WebSimulator.html'
with sync_playwright() as p:
 browser=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox'])
 page=browser.new_page(viewport={'width':1280,'height':950})
 errors=[]
 page.on('pageerror',lambda err:errors.append(str(err)))
 page.set_content(root.read_text().replace('const audio=new TrackAudio();', 'window.__v063={model,rotate,editShortcut,render,TRACK_CORE};const audio=new TrackAudio();',1),wait_until='load',timeout=45000)
 page.wait_for_timeout(1400)
 assert page.locator('#screen').count()==1
 result=page.evaluate('''() => {const {model,rotate,editShortcut,render,TRACK_CORE}=window.__v063;
 model.screen='tracker';model.ch=5;model.col=2;model.row=0;
 const step=TRACK_CORE.step(model.project,0,5,0,true);
 step.note=0;step.chd=3;
 rotate('preset',1);const empty=step.chd;
 rotate('preset',1);const stop=step.chd;
 rotate('preset',1);const restored=step.chd;
 const originalNote1=step.note;
 model.ch=0;model.col=1;editShortcut('groove');
 const a=model.screen;
 rotate('k3',2);const micro=TRACK_CORE.step(model.project,0,0,0,true).groove;
 rotate('k2',1);rotate('k3',2);const ratchet=TRACK_CORE.step(model.project,0,0,0,true).groove;
 rotate('k2',1);rotate('k3',1);const prob=TRACK_CORE.step(model.project,0,0,0,true).groove;
 editShortcut('copyPattern');model.project.selectedPattern=1;editShortcut('pastePattern');
 const copied=TRACK_CORE.step(model.project,1,0,0,false).groove;
 model.project.selectedPattern=0;model.screen='groove';model.grooveFocus=0;
 render();
 return {empty,stop,restored,originalNote1,a,micro,ratchet,prob,copied};
 }''')
 print('Test result:',result)
 assert result['empty']==0 and result['stop']==15 and result['restored']==3
 assert result['originalNote1']==0
 assert result['a']=='groove'
 assert result['micro']==2 and ((result['ratchet']>>3)&3)==2
 assert ((result['prob']>>5)&3)==1 and result['copied']==result['prob']
 # Screenshot previews are generated outside this regression test.
 page.evaluate("window.__v063.model.screen='tracker';window.__v063.model.ch=5;window.__v063.model.col=2;window.__v063.model.project.selectedPattern=0;window.__v063.render()")
 
 assert not errors,errors
 browser.close()
 print('PASS browser V0.63 NOTE2 PRESET / groove editor / copy-paste / no JS errors')
