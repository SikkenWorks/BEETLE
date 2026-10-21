#!/usr/bin/env python3
import base64,re
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[2]
s=(root/'simulator.html').read_text()
m=re.search(r"const inner='([^']+)';",s);assert m
inner=base64.b64decode(m[1]).decode()
inject='''window.__FX_TEST=()=>{
 const p=model.project;const st=step(p,p.selectedPattern,model.ch,model.row,false);return {
 screen:model.screen,slot:model.fxSlot, fx:st.fx,amount:st.amount,
 extras:JSON.parse(JSON.stringify(st.extraFx||[])),pool:fxPoolCount(p),
 note2:JSON.parse(JSON.stringify(st.extraFx2||[])),warning:model.fxFull,
 native:Array.from(BeetleNative.encode(p)).slice(0,8),
 round:JSON.parse(JSON.stringify(BeetleNative.decode(BeetleNative.encode(p)).patterns[p.selectedPattern]?.channels[model.ch]?.[model.row]||{}))
 };};
'''
inner=inner.replace('window.BEETLE_WORKBENCH_BRIDGE={',inject+'window.BEETLE_WORKBENCH_BRIDGE={',1)
s=s[:m.start(1)]+base64.b64encode(inner.encode()).decode()+s[m.end(1):]
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(viewport={'width':1420,'height':900})
 errs=[];page.on('pageerror',lambda e:errs.append(str(e)))
 page.set_content(s,wait_until='domcontentloaded',timeout=60000)
 page.wait_for_function('!!document.getElementById("beetle-engine").contentWindow?.BEETLE_WORKBENCH_BRIDGE',timeout=30000)
 def run(code):return page.evaluate('''code=>{const w=document.getElementById('beetle-engine').contentWindow,b=w.BEETLE_WORKBENCH_BRIDGE;return (new Function('w','b',code))(w,b)}''',code)
 def fx():return run('return w.__FX_TEST()')
 run("b.press('SEL',true);b.press('SEL',false);")
 run("b.page('tracker');b.page('punch');") # FX button
 # ensure stepfx page
 print('initial screen',fx()['screen'])
 run("w.BEETLE_WORKBENCH_BRIDGE.rotate('k1',2);")
 assert fx()['slot']==2,fx()
 run("b.rotate('k2',2);")
 x=fx();assert x['fx']==0 and x['extras'][1]['fx']==2,x
 run("b.rotate('k3',5);")
 x=fx();assert x['extras'][1]['amount']==55,x
 assert x['round']['extraFx'][1]['amount']==55,x
 run("b.rotate('k1',-2);b.rotate('k2',1);")
 assert fx()['fx']==1,fx()
 run("b.press('OCT-',true);b.press('OCT-',false);")
 x=fx();assert x['fx']==0 and x['extras'][1]['fx']==2,x
 run("b.rotate('k1',1);b.rotate('k2',3);")
 x=fx();assert x['extras'][0]['fx']==3 and x['extras'][1]['fx']==2,x
 run("b.rotate('k1',1);b.rotate('k2',-2);")
 x=fx();assert x['extras'][0]['fx']==3 and x['extras'][1] is None,x
 assert not errs,errs
 print('PASS FX3 before FX1 and FX2, K1 selects, K2 effect, K3 value, separate deletion, V22 persistence, no JS errors')
 browser.close()
