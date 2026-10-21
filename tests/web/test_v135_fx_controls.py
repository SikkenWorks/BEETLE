from pathlib import Path
import base64,re
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[2]
s=(root/'simulator.html').read_text()
m=re.search(r"const inner='([^']+)';",s);assert m
engine=base64.b64decode(m[1]).decode()
anchor='window.BEETLE_WORKBENCH_BRIDGE={'
assert anchor in engine
injection='''window.__fxView=()=>({screen:model.screen,row:model.row,slot:model.fxSlot,
 fx1:step(model.project,model.project.selectedPattern,model.ch,model.row,true).fx,
 fx2:step(model.project,model.project.selectedPattern,model.ch,model.row,true).extraFx?.[0]||null,
 fx3:step(model.project,model.project.selectedPattern,model.ch,model.row,true).extraFx?.[1]||null,
 count:fxPoolCount(model.project),schema:BeetleNative.encode(model.project)[4]});
window.__fxLegacy=()=>({project:model.project,codec:BeetleNative});
'''
engine=engine.replace(anchor,injection+anchor,1)
s=s[:m.start(1)]+base64.b64encode(engine.encode()).decode()+s[m.end(1):]
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(viewport={'width':1400,'height':900})
 errors=[];page.on('pageerror',lambda err:errors.append(str(err)))
 page.set_content(s,wait_until='domcontentloaded',timeout=45000)
 page.wait_for_function('!!document.getElementById("beetle-engine").contentWindow?.BEETLE_WORKBENCH_BRIDGE',timeout=30000)
 def exec(js):
  return page.evaluate('''j=>{const w=document.getElementById('beetle-engine').contentWindow,b=w.BEETLE_WORKBENCH_BRIDGE;return (function(){'''+js+'''})();}''','')
 def view():return page.evaluate("document.getElementById('beetle-engine').contentWindow.__fxView()")
 exec("b.press('SEL',true);b.press('SEL',false);b.page('tracker');b.press('FX',true);b.press('FX',false);")
 assert view()['screen']=='stepfx',view()
 exec("b.rotate('k1',2)")
 assert view()['slot']==2,view()
 exec("b.rotate('k2',2)")
 r=view();assert r['fx1']==0 and r['fx2'] is None and r['fx3']['fx']==2 and r['count']==1,r
 for i in range(5):
  exec("b.rotate('k3',1)")
  print('amount',i,view()['fx3']['amount'])
 assert view()['fx3']['amount']==55,view()
 exec("b.rotate('k1',-1);b.rotate('k2',1)")
 r=view();assert r['fx2']['fx']==1 and r['fx3']['fx']==2 and r['fx1']==0,r
 exec("b.rotate('k1',-1);b.rotate('k2',5)")
 r=view();assert r['fx1']==5 and r['fx2']['fx']==1 and r['fx3']['fx']==2 and r['count']==2,r
 assert r['schema']==22,r
 exec("b.rotate('k1',1);b.press('OCT-',true);b.press('OCT-',false)")
 r=view();assert r['fx2'] is None and r['fx3']['fx']==2 and r['count']==1,r
 assert not errors,errors
 print('PASS V1.3.5 UI: K1 row, K2 effect, K3 value, FX3 independent, all slots, isolated delete, V22, no JS errors')
 browser.close()
