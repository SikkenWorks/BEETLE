from playwright.sync_api import sync_playwright
from pathlib import Path
import json
p=Path(__file__).resolve().parents[3]/'simulator.html'
with sync_playwright() as x:
 b=x.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox'])
 ctx=b.new_context(viewport={'width':1280,'height':900},accept_downloads=True)
 page=ctx.new_page()
 errors=[]
 page.on('pageerror',lambda e: errors.append(str(e)))
 page.set_content(p.read_text(),wait_until='load',timeout=30000)
 page.wait_for_timeout(1600)
 page.locator('[data-action="SEL"]').click()
 page.wait_for_timeout(90)
 def at(r,col):
  box=page.locator('#screen').bounding_box(); assert box
  x=[0,37,81,122,204][col]
  page.mouse.click(box['x']+box['width']*x/240,box['y']+box['height']*(48+10*r)/240)
 def backup():
  with page.expect_download() as d:
   page.locator('#backupButton').click()
  download=d.value
  path=Path('/tmp/beetle-v085-browser-backup-'+str(len(backups))+'.json')
  download.save_as(path)
  obj=json.loads(path.read_text())
  backups.append(obj)
  return obj['project']
 backups=[]
 for r,col in enumerate([1,2,3,4]):
  at(r,col)
  page.locator('.white-key').first.dispatch_event('pointerdown',{'button':0,'buttons':1,'pointerId':20+r,'pointerType':'mouse'})
  page.locator('.white-key').first.dispatch_event('pointerup',{'button':0,'buttons':0,'pointerId':20+r,'pointerType':'mouse'})
  page.wait_for_timeout(25)
 proj=backup(); chan=proj['patterns']['0']['channels'][0]
 print('ROWS',[(i,chan[i]['note'],chan[i]['inst'],chan[i]['chd'],chan[i]['fx']) for i in range(4)])
 assert all(chan[i]['note']==60 for i in range(4)), 'Not all columns insert into NOTE!'
 for _ in range(4):page.locator('[data-turn="k1:-1"]').click()
 page.locator('[data-action="SEQ"]').dispatch_event('pointerdown',{'button':0,'buttons':1,'pointerId':45,'pointerType':'mouse'})
 page.locator('[data-action="OCT+"]').dispatch_event('pointerdown',{'button':0,'buttons':1,'pointerId':46,'pointerType':'mouse'})
 page.locator('[data-action="OCT+"]').dispatch_event('pointerup',{'button':0,'buttons':0,'pointerId':46,'pointerType':'mouse'})
 page.locator('[data-action="SEQ"]').dispatch_event('pointerup',{'button':0,'buttons':0,'pointerId':45,'pointerType':'mouse'})
 page.wait_for_timeout(150)
 proj2=backup(); c2=proj2['patterns']['0']['channels'][0]
 assert c2[0]['note']==72, ('Transpose failed',c2[0])
 assert all(c2[i]['note']==60 for i in range(1,4)), 'Other notes changed'
 assert chan[0]['inst']==c2[0]['inst'] and chan[0]['chd']==c2[0]['chd'] and chan[0]['fx']==c2[0]['fx'], 'Other fields altered'
 assert 'active' not in (page.locator('[data-action="OCT+"]').get_attribute('class') or ''), 'SEQ+OCT changed keyboard octave'
 assert not errors,errors
 print('CHROMIUM FUNCTIONAL PASS: four-column entry, step transpose/preview, no other field damage, live octave unchanged')
 b.close()
