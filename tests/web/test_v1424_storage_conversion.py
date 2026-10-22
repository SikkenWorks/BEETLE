from playwright.sync_api import sync_playwright
from pathlib import Path
p=str((Path(__file__).resolve().parents[2]/'BEETLE-Project-Manager.html').resolve())
with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(accept_downloads=True)
 errors=[]
 page.on('pageerror',lambda ex:errors.append(str(ex)))
 page.set_content(Path(p).read_text(),wait_until='load')
 assert page.locator('#convertStorageV2').count()==1
 assert page.locator('#convertStorageV2').is_disabled()
 print('DOM migration button present and initially disabled')
 # simulate USB project transfer / migration; no actual hardware
 result=page.evaluate('''async()=>{
  const projects=[{slot:0,raw:new Uint8Array([1,2,3,4,5,6,7,8]),info:{version:22,slot:0,name:'MOUNTAIN'}},
    {slot:6,raw:new Uint8Array([91,16,63,42]),info:{version:23,slot:6,name:'NUGGIE'}}];
  window.__actions=[];
  connected=true;deviceProtocol=5;deviceSchema=24;storageMigrationMode='legacy';deviceGlobalSamples=false;
  for(let i=0;i<12;i++)slots[i]={state:projects.some(p=>p.slot===i)?'used':'empty',name:projects.find(p=>p.slot===i)?.info.name||'',version:22};
  const mask=projects.reduce((v,p)=>v|(1<<p.slot),0),digest=fingerprintBackup(projects);
  let mode=0,restores=0,downloaded=0;
  query=async(cmd,args)=>{
   window.__actions.push({cmd,args});
   if(cmd===0x5d)return Uint8Array.from([mode,mask&255,mask>>8,...le32(digest)]);
   if(cmd===0x5e){if(!downloaded)throw Error('FLASH REQUESTED BEFORE BACKUP');mode=1;return Uint8Array.of(1);}
   throw Error('unexpected command '+cmd);
  };
  modal=async(title)=>title.includes('STEP 1')?'backup':title.includes('STEP 2')?'yes':'ok';
  encryptProject=async raw=>new Blob([raw]);download=()=>{downloaded++};
  readProjectFromSlot=async slot=>{let p=projects.find(p=>p.slot===slot);if(!p)throw Error('not filled');
   return {...p,info:{...p.info,version:mode?24:p.info.version}};
  };
  uploadProject=async (slot,raw)=>{restores++;slots[slot]={state:'used',name:projects.find(p=>p.slot===slot).info.name,version:24};};
  refresh=async()=>{for(let i=0;i<12;i++)slots[i]=projects.some(p=>p.slot===i)?{state:'used',name:projects.find(p=>p.slot===i).info.name,version:24}:{state:'empty',name:'',version:null};};
  await convertStorageV2();
  return {actions:window.__actions.map(x=>x.cmd),restores,downloaded,mode,storageMigrationMode,error:window.__error};
 }''')
 print(result)
 assert result['mode']==1 and result['restores']==2 and result['downloaded']==1
 assert result['actions'].count(0x5e)==1 and result['actions'].count(0x5d)==3
 assert result['actions'].index(0x5e)>result['actions'].index(0x5d)
 assert not errors,errors
 print('PASS end-to-end UI test simulated 12 slots, 2 projects, V2 format and restore')
 browser.close()
# Independent negative preflight test: an altered digest MUST NOT send 0x5E.
with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page()
 page.set_content(Path(p).read_text(),wait_until='load')
 outcome=page.evaluate('''async()=>{
  const raw=new Uint8Array([1,2,3,4]),projects=[{slot:0,raw,info:{version:22,slot:0,name:'TEST'}}];
  connected=true;deviceProtocol=5;deviceSchema=24;storageMigrationMode='legacy';deviceGlobalSamples=false;
  for(let i=0;i<12;i++)slots[i]={state:i===0?'used':'empty',name:i===0?'TEST':'',version:22};
  let calls=[],checks=0,downloads=0;
  query=async(cmd,args)=>{calls.push(cmd);if(cmd===0x5d){checks++;return Uint8Array.from([0,1,0,...le32(fingerprintBackup(projects)+(checks>=2?1:0))]);}if(cmd===0x5e)throw Error('unsafe erase reached');throw Error('unexpected');};
  readProjectFromSlot=async()=>projects[0];modal=async title=>title.includes('STEP 1')?'backup':title.includes('STEP 2')?'yes':'ok';
  encryptProject=async raw=>new Blob([raw]);download=()=>downloads++;
  let error=null;try{await convertStorageV2();}catch(e){error=e.message;}
  return {calls,checks,downloads,error};
 }''')
 assert outcome['calls']==[93,93] and outcome['downloads']==1, outcome
 print('PASS changed project fingerprint refused before any flash command')
 browser.close()
with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page()
 page.set_content(Path(p).read_text(),wait_until='load')
 outcome=page.evaluate('''async()=>{
  connected=true;deviceProtocol=5;deviceSchema=24;storageMigrationMode='v2';deviceGlobalSamples=true;
  const original=new Uint8Array(42);const dv=new DataView(original.buffer);original.set([84,82,75,49],0);dv.setUint16(4,22,true);dv.setUint16(6,5,true);dv.setUint32(8,42,true);original.set([83,73,88],24);original[36]=5;dv.setUint32(12,crc32(original.subarray(24)),true);dv.setUint32(16,crc32(original.subarray(0,16)),true);
  const item={slot:5,raw:original,info:{slot:5,name:'SIX',version:22}};
  const manifest={format:'BEETLE-V2-MIGRATION-BACKUP-1',device_fingerprint:fingerprintBackup([item]),slots:Array.from({length:12},(_,i)=>i!==5?{slot:i+1,empty:true}:{slot:6,empty:false,name:'SIX',length:original.length,crc32:crc32(original)})};
  const files=[{name:'SIX-BEFORE-V2-SLOT-06.beetle',bytes:original},{name:'MIGRATION-MANIFEST.json',bytes:enc.encode(JSON.stringify(manifest))}];
  const input=new Blob([makeBackupZip(files)]);
  BeetleFileV2.decrypt=async blob=>new Uint8Array(await blob.arrayBuffer());
  let uploaded=[];modal=async()=> 'yes';
  uploadProject=async(slot,raw)=>{uploaded.push({slot,raw:raw.length});slots[slot]={state:'used',version:24,name:'SIX'};};
  readProjectFromSlot=async slot=>({raw:original,info:{slot,name:'SIX',version:24}});
  refresh=async()=>{for(let i=0;i<12;i++)slots[i]=i===5?{state:'used',version:24,name:'SIX'}:{state:'empty',name:'',version:null}};
  await restoreV2FromZip(input);
  return {uploaded,state:storageMigrationMode};
 }''')
 assert outcome['uploaded']==[{'slot':5,'raw':42}],outcome
 print('PASS resume from encrypted-backup ZIP fixture; no second flash format')
 browser.close()
