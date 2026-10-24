#!/usr/bin/env python3
"""Browser mock test: FM1 read-only capacity and full-storage blocking.
Does not connect to a real FM1."""
import asyncio,sys
from pathlib import Path
from playwright.async_api import async_playwright
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).parent))
from test_v1425_sample_inventory_safesync import INIT,wav_bytes
MOCK=INIT.replace('case 0x71:rc=0x79;', '''case 0x76:{
  if(window.__capacityUnavailable){rc=0x6f;res=[seq,cmd,9];break;}
  rc=0x7e;
  const used=window.__mockSlots.reduce((a,b)=>a+(b.present?b.bytes:0),0);
  res=[seq,1,...le(used),...le(window.__freeBytes ?? 90000),25,8,4,0,0];break;
 }
 case 0x71:rc=0x79;''')
async def run():
 async with async_playwright() as p:
  browser=await p.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage'])
  page=await browser.new_page();errors=[]
  page.on('pageerror',lambda e:errors.append(str(e)))
  await page.set_content((ROOT/'BEETLE-Project-Manager.html').read_text())
  await page.evaluate(MOCK)
  await page.evaluate("Object.defineProperty(window,'isSecureContext',{configurable:true,value:true})")
  await page.locator('#connect').click()
  await page.wait_for_function("document.querySelector('#sampleCapacityPercent').textContent.includes('% USED')")
  d=await page.locator('#sampleCapacityDetails').inner_text()
  assert 'ESTIMATED REMAINING' in d,d
  assert '32.0 KiB' in await page.locator('#sampleCapacityReserve').inner_text()
  # With a full FM1 bar, uploads are blocked before 0x71 BEGIN.
  await page.evaluate('window.__freeBytes=0')
  await page.locator('#bankDeviceRefresh').click()
  await page.wait_for_function("document.querySelector('#sampleCapacityPercent').textContent.includes('FULL')")
  await page.locator('#bankRows > div').nth(2).get_by_role('button',name='IMPORT WAV').click()
  await page.locator('#bankWav').set_input_files({'name':'sample.wav','mimeType':'audio/wav','buffer':wav_bytes()})
  n=len([e for e in await page.evaluate('window.__calls') if e['cmd']==0x71])
  await page.locator('#bankTransfer').click()
  await page.locator('#modalActions').get_by_role('button',name='OK').wait_for()
  assert 'FM1 STORAGE FULL' in await page.locator('#modalText').inner_text()
  assert n==len([e for e in await page.evaluate('window.__calls') if e['cmd']==0x71]),'Upload started while full'
  await page.locator('#modalActions').get_by_role('button',name='OK').click()
  # Legacy fallback: no fabricated percentage, but earlier behaviour retained.
  await page.evaluate('window.__capacityUnavailable=true')
  await page.locator('#bankDeviceRefresh').click()
  await page.wait_for_function("document.querySelector('#sampleCapacityPercent').textContent.includes('NOT VERIFIED')")
  assert 'UNKNOWN' in await page.locator('#sampleCapacityDetails').inner_text()
  assert '32.0 KiB' not in await page.locator('#sampleCapacityReserve').inner_text()
  assert not errors,errors
  await browser.close()
  print('PASS: 32KiB reserve meter, genuine capacity data, full-bar block before write, honest V1.4.24 fallback')
asyncio.run(run())
