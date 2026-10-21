"""Browser-only read-only ownership audit / explicit marker repair, mock USB MIDI."""
from pathlib import Path
from playwright.sync_api import sync_playwright
html=(Path(__file__).resolve().parents[2]/'web'/'BEETLE-Project-Manager.html').read_text()
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 for scenario in ('repairable','foreign'):
  page=browser.new_page(viewport={'width':1280,'height':1000});errs=[]
  page.on('pageerror',lambda e:errs.append(str(e)))
  page.set_content(html)
  page.evaluate('''(scenario)=>{
   Object.defineProperty(window,'isSecureContext',{configurable:true,value:true});
   window.auditCode=scenario==='repairable'?1:2;window.repaired=0;window.scans=0;
   const inp={id:'A',name:'FM1',onmidimessage:null,open:async()=>{},close:async()=>{}};
   const out={id:'B',name:'FM1',open:async()=>{},close:async()=>{},send(data){
    const request=parseFrame(data),seq=request.body[0];let cmd,body;
    if(request.command===0x50){cmd=0x60;body=[seq,4,12,17,1]}
    else if(request.command===0x58){cmd=0x64;body=[seq]}
    else if(request.command===0x51){window.scans++;
     if(window.auditCode){cmd=0x6f;body=[seq,0x51,11]}
     else{cmd=0x61;body=[seq,request.body[1],0,...Array(12).fill(32)]}}
    else if(request.command===0x5b){cmd=0x6b;body=[seq,window.auditCode]}
    else if(request.command===0x5c){if(window.auditCode!==1){cmd=0x6f;body=[seq,0x5c,11]}
     else{window.repaired++;window.auditCode=0;cmd=0x6c;body=[seq,1]}}
    else{cmd=0x6f;body=[seq,request.command,10]}
    setTimeout(()=>inp.onmidimessage?.({data:frame(cmd,body)}),0);
   }};
   Object.defineProperty(navigator,'requestMIDIAccess',{configurable:true,value:async()=>({inputs:new Map([['A',inp]]),outputs:new Map([['B',out]])})});
  }''',scenario)
  page.locator('#connect').click()
  page.wait_for_function("document.querySelector('#connectionState').textContent==='FM1 CONNECTED'",timeout=15000)
  page.locator('#diagnoseMarker').click()
  page.wait_for_function("document.querySelector('#diagnosticMeta').textContent.includes('STORAGE MARKER')",timeout=15000)
  if scenario=='repairable':
   assert page.locator('#repairMarker').is_enabled()
   page.locator('#repairMarker').click()
   page.get_by_role('button',name='CONTINUE').click()
   page.locator('#modalExtra input').check()
   page.get_by_role('button',name='REPAIR VERIFIED MARKER').click()
   page.wait_for_function("document.querySelector('#diagnosticMeta').textContent.includes('MARKER REPAIRED')",timeout=25000)
   assert page.evaluate('window.repaired')==1
   assert page.evaluate('window.scans')>=13
   print('PASS missing marker audit, explicit opt-in, repair, verification, slot rescan')
  else:
   assert page.locator('#repairMarker').is_disabled()
   assert page.evaluate('window.repaired')==0
   print('PASS foreign data refused without repair command')
  assert not errs,errs
  page.close()
 browser.close()
