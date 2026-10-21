"""Browser-only mocked WebMIDI checks. No physical FM1 and no flashing."""
from pathlib import Path
from playwright.sync_api import sync_playwright
h=(Path(__file__).resolve().parents[2]/'web'/'BEETLE-Project-Manager.html').read_text()
MOCK_SETUP='''(scenario) => {
 Object.defineProperty(window,'isSecureContext',{configurable:true,value:true});
 window.__sent=[];
 const inp={id:'A',name:'FM1',onmidimessage:null,open:async()=>{},close:async()=>{}};
 const out={id:'B',name:'FM1',open:async()=>{},close:async()=>{},send(data){
  const req=parseFrame(data);window.__sent.push(req.command);
  let cmd,body;
  if(scenario==='legacy' && req.command===0x11){cmd=0x11;body=Array.from(new TextEncoder().encode('FM-1_2101'));}
  if(scenario==='enabled'){
   const seq=req.body[0];
   if(req.command===0x50){cmd=0x60;body=[seq,1,12,17,1];}
   else if(req.command===0x58){cmd=0x64;body=[seq];}
   else if(req.command===0x51){cmd=0x61;body=[seq,req.body[1],0,...Array(12).fill(32)];}
  }
  if(cmd!==undefined)setTimeout(()=>inp.onmidimessage?.({data:frame(cmd,body)}),0);
 }};
 Object.defineProperty(navigator,'requestMIDIAccess',{configurable:true,value:async()=>({inputs:new Map([['A',inp]]),outputs:new Map([['B',out]])})});
}'''
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 for scenario,expected in [('enabled','CONNECTED · 12 SLOTS'),('legacy','FIRMWARE WITHOUT PROJECT TRANSFER'),('silent','NO USB REPLY')]:
  page=browser.new_page(viewport={'width':1280,'height':960});errors=[]
  page.on('pageerror',lambda x:errors.append(str(x)))
  page.set_content(h);page.evaluate(f'({MOCK_SETUP})({scenario!r})')
  page.locator('#connect').click()
  page.wait_for_function(f'document.querySelector("#diagnosticMeta").textContent.includes({expected!r})',timeout=12000)
  assert not errors,(scenario,errors)
  cmds=page.evaluate('window.__sent')
  if scenario=='enabled':
   assert cmds[:2]==[0x50,0x58] and cmds.count(0x51)==12,cmds
   assert page.locator('#used').inner_text()=='00'
  else:
   assert cmds==[0x50,0x11],cmds
   assert page.locator('#connectionState').inner_text()=='NOT CONNECTED'
   assert page.locator('#refresh').is_disabled()
   assert page.locator('#diagnosticResult').inner_text()
  print('PASS',scenario,'=>',expected,'requests',len(cmds))
  page.close()
 # Basic responsive sizing, no overflow.
 for width in (400,1280):
  page=browser.new_page(viewport={'width':width,'height':900});page.set_content(h)
  assert page.evaluate('document.documentElement.scrollWidth <= window.innerWidth + 1'),width
  print('PASS responsive',width)
  page.close()
 browser.close()
