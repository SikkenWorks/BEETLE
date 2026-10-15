"""BEETLE V0.84 Chromium regression for new master/boot navigation."""
from pathlib import Path
from playwright.sync_api import sync_playwright
r=Path(__file__).resolve().parents[2]
html=(r.parent/'simulator.html').read_text().replace('function whiteDown(i){',
 'window.__v84test={model,rotate,route,core:TRACK_CORE};function whiteDown(i){')
with sync_playwright() as pw:
 b=pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=b.new_page(viewport={'width':1200,'height':800})
 errors=[]
 page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(html)
 page.wait_for_timeout(300)
 x=page.evaluate('''() => {
  const {model,rotate,route}=window.__v84test;
  model.screen='masterfx';model.masterFocus=0;
  const m=model.project.master;m.enabled=[true,true,true];
  rotate('k1',1);const focus=model.masterFocus;
  const before=m.comp_threshold;rotate('select',2);
  route('SEL');const after=m.enabled[1];
  model.screen='load';model.loadFromBoot=true;route('HOME');const boot=model.screen;
  model.screen='load';model.loadFromBoot=false;route('HOME');const normal=model.screen;
  return {focus,before,adjusted:m.comp_threshold,after,boot,normal};
 }''')
 assert x['focus']==1,x
 assert x['adjusted']==x['before']+2,x
 assert x['after'] is False,x
 assert x['boot']=='boot',x
 assert x['normal']=='projects',x
 assert not errors,errors
 print('PASS V0.84 WebSimulator: K1 selects master, SELECT parameter, SEL enables, boot HOME routes correctly')
 b.close()
