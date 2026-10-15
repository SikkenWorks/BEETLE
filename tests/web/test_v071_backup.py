#!/usr/bin/env python3
"""BEETLE V0.72 browser backup and damage rejection tests."""
from pathlib import Path
from playwright.sync_api import sync_playwright
p=Path(__file__).resolve().parents[2]/'web/simulator.html'
with sync_playwright() as pw:
 b=pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 page=b.new_page(viewport={'width':1250,'height':1000},accept_downloads=True)
 errors=[]
 page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(p.read_text())
 page.wait_for_timeout(1300)
 result=page.evaluate('''async () => {
   const {beetleMakeBackup,beetleReadBackup,beetleHash,beetleSha256Portable}=window.__beetleBackupTest;
   const p=TRACK_CORE.freshProject('TEST BACKUP');
   p.patterns[0]=TRACK_CORE.emptyPattern();p.patterns[0].channels[0][0].note=60;
   p.library[0]={name:'SID BASS',patch:{...p.instruments[0]}};
   p.snapshots=[3,null,7,null];p.muteGroups=[3,8,0,0];
   if(beetleSha256Portable(new TextEncoder().encode('abc'))!=='ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad')throw Error('SHA256 self-test failed');
   const original=JSON.stringify(p);
   const backup=await beetleMakeBackup(p);
   const raw=JSON.stringify(backup);
   const restored=await beetleReadBackup(raw);
   const legacy=await beetleReadBackup(original);
   let checksumRejected=false,shapeRejected=false,limitRejected=false;
   const modified=JSON.parse(raw);modified.project.bpm=171;
   try{await beetleReadBackup(JSON.stringify(modified));}catch(e){checksumRejected=/checksum/i.test(e.message);}
   const malformed=JSON.parse(raw);malformed.project.patterns[0].channels[0].pop();
   malformed.contentBytes=new TextEncoder().encode(JSON.stringify(malformed.project)).length;
   malformed.sha256=await beetleHash(JSON.stringify(malformed.project));
   try{await beetleReadBackup(JSON.stringify(malformed));}catch(e){shapeRejected=/steps/i.test(e.message);}
   try{await beetleReadBackup(' '.repeat(2097152+4097));}catch(e){limitRejected=/safety limit/i.test(e.message);}
   return {sha256:backup.sha256,roundtrip:JSON.stringify(restored.project)===original,
           library:restored.project.library[0].name,legacy:legacy.verified,
           verified:restored.verified,checksumRejected,shapeRejected,limitRejected};
 }''')
 assert result['roundtrip'],result
 assert result['verified'] is True and result['legacy'] is False,result
 assert result['library']=='SID BASS' and len(result['sha256'])==64,result
 assert result['checksumRejected'] and result['shapeRejected'] and result['limitRejected'],result
 # UI smoke: real backup button creates a complete envelope even offline.
 with page.expect_download(timeout=10000) as downloading:
  page.locator('#backupButton').click()
 download=downloading.value
 assert download.suggested_filename.endswith('.beetle-backup.json'),download.suggested_filename
 import json,hashlib
 with __import__('tempfile').TemporaryDirectory() as tmp:
  location=Path(tmp)/download.suggested_filename
  download.save_as(location)
  data=json.loads(location.read_text())
  canonical=json.dumps(data['project'],separators=(',',':'),ensure_ascii=False).encode('utf-8')
  assert hashlib.sha256(canonical).hexdigest()==data['sha256']
 assert not errors,errors
 print('PASS V0.72 browser: SHA-256 roundtrip, library preservation, legacy import, tamper/shape/size refusals')
 b.close()
