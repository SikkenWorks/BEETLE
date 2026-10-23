import asyncio, io, wave, struct
from pathlib import Path
from playwright.async_api import async_playwright

ROOT=Path(__file__).resolve().parents[2]
INIT='''() => {
window.__mockSlots = Array.from({length:13},(_,i)=>i<2?{present:true,bytes:1234+i,crc:0x10203040+i,generation:11}: {present:false,bytes:0,crc:0,generation:0});
window.__calls=[];
let session=null;
const inp={id:'mockin',name:'M-VAVE FM1 (Mock IN)',type:'input',onmidimessage:null,open:async()=>{},close:async()=>{}};
const out={id:'mockout',name:'M-VAVE FM1 (Mock OUT)',type:'output',open:async()=>{},close:async()=>{},send: data=>{
 const request=window.parseFrame(data),cmd=request.command,args=request.body, seq=args[0],p=args.slice(1);
 window.__calls.push({cmd,slot:p[0]});
 let rc=0x6f,res=[seq,0,9];
 const le=(n)=>[n&255,(n>>>8)&255,(n>>>16)&255,(n>>>24)&255];
 switch(cmd){
 case 0x50:rc=0x60;res=[seq,5,12,25,3];break;
 case 0x58:rc=0x64;res=[seq,0];break;
 case 0x51:rc=0x61;res=[seq,p[0],0,...new Array(12).fill(0)];break;
 case 0x70:{if(window.__blockInfoSlot===p[0]){rc=0x6f;res=[seq,cmd,9];break;}rc=0x78;const s=window.__mockSlots[p[0]];res=[seq,p[0],s.present?1:0,...le(s.bytes),...le(s.crc),...le(s.generation)];break;}
 case 0x71:rc=0x79;session={slot:p[0],size:p[1]|p[2]<<8|p[3]<<16|p[4]<<24,crc:(p[5]|p[6]<<8|p[7]<<16|p[8]<<24)>>>0,data:[]};res=[seq,p[0]];break;
 case 0x72:{rc=0x7a;const off=(p[0]|p[1]<<8|p[2]<<16|p[3]<<24)>>>0;if(!session||off!==session.data.length)throw Error('BAD TRANSFER OFFSET');session.data.push(...p.slice(5,5+p[4]));res=[seq,...le(session.data.length)];break;}
 case 0x73:{if(!session||session.size!==session.data.length)throw Error('BAD COMMIT SIZE'); const actual=window.BeetleGlobalBank.crc32(Uint8Array.from(session.data));if(actual!==session.crc)throw Error('BAD COMMIT CRC');rc=0x7b;res=[seq,session.slot];window.__mockSlots[session.slot]={present:true,bytes:session.size,crc:actual,generation:20};session=null;break;}
 case 0x74:rc=0x7c;session=null;res=[seq];break;
 case 0x75:{if(p[1]!==0xd3||p[2]!==0x7e)throw Error('BAD DELETE');rc=0x7d;window.__mockSlots[p[0]]={present:false,bytes:0,crc:0,generation:0};res=[seq,p[0]];break;}
 }
 queueMicrotask(()=>inp.onmidimessage?.({data:window.frame(rc,res)}));
 }};
 Object.defineProperty(navigator,'requestMIDIAccess',{configurable:true,value:async()=>({inputs:new Map([[inp.id,inp]]),outputs:new Map([[out.id,out]])})});
}'''

def wav_bytes():
 out=io.BytesIO()
 with wave.open(out,'wb') as w:
  w.setnchannels(1);w.setsampwidth(2);w.setframerate(22050)
  w.writeframes(b''.join(struct.pack('<h',int(10000*((i%24)/23*2-1))) for i in range(550)))
 return out.getvalue()

async def test():
 async with async_playwright() as pw:
  browser=await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage'])
  page=await browser.new_page()
  # content is inlined because the execution environment blocks browser navigation
  
  errors=[]
  page.on('pageerror',lambda e:errors.append(str(e)))
  await page.set_content((ROOT/'BEETLE-Project-Manager.html').read_text(encoding='utf-8'))
  await page.evaluate(INIT)
  await page.evaluate("Object.defineProperty(window,'isSecureContext',{configurable:true,value:true})")
  await page.locator('#connect').click()
  await page.wait_for_function('window.__calls.filter(c=>c.cmd===0x70).length >= 13')
  await page.wait_for_function("document.querySelector('#bankDeviceStatus').textContent.includes('2 STORED')")
  row0=page.locator('#bankRows > div').nth(0)
  row1=page.locator('#bankRows > div').nth(1)
  assert 'STORED ON FM1' in await row0.inner_text()
  assert 'STORED ON FM1' in await row1.inner_text()
  assert await page.locator('#bankTransfer').is_disabled()
  # Add an individual sample to PC bank slot U03; device U01/U02 remain untouched
  row2=page.locator('#bankRows > div').nth(2)
  await row2.get_by_role('button',name='IMPORT WAV').click()
  await page.locator('#bankWav').set_input_files({'name':'new03.wav','mimeType':'audio/wav','buffer':wav_bytes()})
  await page.wait_for_function("document.querySelector('#bankStats').textContent.includes('1 / 13')")
  assert await page.locator('#bankTransfer').is_enabled()
  await page.locator('#bankTransfer').click()
  await page.locator('#modalActions').get_by_role('button',name='UPLOAD LISTED SLOTS').wait_for()
  message=await page.locator('#modalText').inner_text()
  assert 'U03' in message and 'PRESERVED/UNTOUCHED: 12' in message
  await page.locator('#modalExtra input[type=checkbox]').check()
  await page.locator('#modalActions').get_by_role('button',name='UPLOAD LISTED SLOTS').click()
  await page.wait_for_function("document.querySelector('#bankDeviceStatus').textContent.includes('3 STORED')")
  state=await page.evaluate('window.__mockSlots')
  assert state[0]['generation']==11 and state[1]['generation']==11 and state[2]['present']
  calls=await page.evaluate('window.__calls')
  assert not any(c['cmd']==0x75 for c in calls)
  assert [c['slot'] for c in calls if c['cmd']==0x71]==[2]
  # Reconnect: all slots must show STORED without loading a PC bank
  await page.locator('#connect').click()
  await page.locator('#connect').click()
  await page.wait_for_function("document.querySelector('#bankDeviceStatus').textContent.includes('3 STORED')")
  assert 'STORED ON FM1' in await row0.inner_text()
  # Remove local copy, then never erase device slot during an upload
  await page.locator('#bankRows > div').nth(2).get_by_role('button',name='REMOVE PC WAV').click()
  await page.locator('#modalActions').get_by_role('button',name='DELETE SAMPLE').click()
  assert await page.locator('#bankTransfer').is_disabled()
  assert await page.evaluate('window.__mockSlots[2].present')
  # Explicit remove FM1 protected by confirmation
  await page.locator('#bankRows > div').nth(1).get_by_role('button',name='DELETE ON FM1').click()
  assert 'DELETE FM1 SAMPLE U02' in await page.locator('#modalTitle').inner_text()
  await page.locator('#modalActions').get_by_role('button',name='CANCEL').click()
  assert await page.evaluate('window.__mockSlots[1].present')
  await page.locator('#bankRows > div').nth(1).get_by_role('button',name='DELETE ON FM1').click()
  await page.locator('#modalExtra input[type=checkbox]').check()
  await page.locator('#modalActions').get_by_role('button',name='DELETE FROM FM1').click()
  await page.wait_for_function("document.querySelector('#bankDeviceStatus').textContent.includes('2 STORED')")
  assert not await page.evaluate('window.__mockSlots[1].present')
  assert await page.evaluate('window.__mockSlots[0].present')
  # Replacement requires a dedicated confirmation and cannot touch other samples.
  await page.locator('#bankRows > div').nth(0).get_by_role('button',name='IMPORT WAV').click()
  await page.locator('#bankWav').set_input_files({'name':'replacement.wav','mimeType':'audio/wav','buffer':wav_bytes()})
  await page.wait_for_function("document.querySelector('#bankStats').textContent.includes('1 / 13')")
  commits_before=len([c for c in await page.evaluate('window.__calls') if c['cmd']==0x73])
  await page.locator('#bankTransfer').click()
  await page.locator('#modalActions').get_by_role('button',name='CANCEL').click()
  assert len([c for c in await page.evaluate('window.__calls') if c['cmd']==0x73])==commits_before
  await page.locator('#bankTransfer').click()
  assert 'REPLACING FM1 SAMPLES: U01' in await page.locator('#modalText').inner_text()
  await page.locator('#modalExtra input[type=checkbox]').check()
  await page.locator('#modalActions').get_by_role('button',name='REPLACE LISTED SLOTS').click()
  await page.wait_for_function("document.querySelector('#bankDeviceStatus').textContent.includes('2 STORED')")
  after=await page.evaluate('window.__mockSlots')
  assert after[0]['generation']==20 and after[2]['present']
  assert not any(c['cmd']==0x75 and c['slot']!=1 for c in await page.evaluate('window.__calls'))
  # Incomplete scans disable writes; no unsafe fallback to empty.
  await page.evaluate('window.__blockInfoSlot=4')
  await page.locator('#bankDeviceRefresh').click()
  await page.locator('#modalActions').get_by_role('button',name='OK').wait_for()
  await page.locator('#modalActions').get_by_role('button',name='OK').click()
  assert await page.locator('#bankTransfer').is_disabled()
  assert 'UNKNOWN' in await page.locator('#bankDeviceStatus').inner_text()
  await page.evaluate('window.__blockInfoSlot=null')
  await page.locator('#bankDeviceRefresh').click()
  await page.wait_for_function("document.querySelector('#bankDeviceStatus').textContent.includes('2 STORED')")
  print('PASS: FM1 inventory, preserve all unselected slots, no implicit deletes, reconnect, explicit delete, protected replacements, cancellation, incomplete-scan write blocking')
  if errors: raise AssertionError('BROWSER JS ERRORS: '+str(errors))
  await browser.close()

if __name__=='__main__':asyncio.run(test())
