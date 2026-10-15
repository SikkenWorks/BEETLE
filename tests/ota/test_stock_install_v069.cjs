'use strict';
const fs=require('fs'),vm=require('vm'),assert=require('assert'),crypto=require('crypto'),path=require('path');
const html=fs.readFileSync(path.resolve(__dirname,'../../web/BEETLE-V0.69-WebInstaller-EXPERIMENTAL.html'),'utf8');
const match=html.match(/<script>(\/\* SPDX-License-Identifier: GPL-3\.0-only[\s\S]*?)<\/script>/);
assert(match,'installer core script exists');
const moduleFake={exports:{}};
const sandbox={module:moduleFake,console,Uint8Array,ArrayBuffer,Set,Date,Math,Number,String,Error,Promise,
  setTimeout:(callback,ms)=>Promise.resolve().then(callback),
  crypto:crypto.webcrypto,atob:globalThis.atob};
sandbox.globalThis=sandbox;
vm.runInNewContext(match[1],sandbox,{filename:'installer_core.js'});
const a=moduleFake.exports;
assert.equal(a.C.ID_ET,'FM-1_2069');
assert.equal(a.isSupportedEtchIdentity('FM-1_015'),true,'official stock V15 allowed');
assert.equal(a.isSupportedEtchIdentity('ota-FM-1_015'),true,'known OTA resume allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2044'),true,'previous firmware still allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2045'),true,'previous 0.45 firmware still allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2046'),true,'previous firmware allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2047'),true,'previous V0.47 firmware allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2048'),true,'previous V0.48 firmware allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2049'),true,'previous V0.49 app expected as predecessor');
assert.equal(a.isSupportedEtchIdentity('FM-1_2050'),true,'V0.50 firmware allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2051'),true,'V0.51 predecessor allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2052'),true,'V0.52 predecessor allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2053'),true,'previous V0.53 firmware allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2054'),true,'previous V0.54 firmware allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2055'),true,'V0.55 predecessor allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2056'),true,'V0.56 predecessor allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2057'),true,'V0.57 predecessor allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2058'),true,'previous V0.58 firmware allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2059'),true,'previous V0.59 firmware allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2060'),true,'V0.60 predecessor allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2061'),true,'V0.61 predecessor allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2062'),true,'V0.62 predecessor allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2063'),true,'previous V0.63 firmware allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2064'),true,'V0.64 predecessor allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2065'),true,'previous firmware allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2066'),true,'V0.66 predecessor allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2067'),true,'V0.67 predecessor allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_2069'),true,'current firmware allowed');
assert.equal(a.isSupportedEtchIdentity('FM-1_1140'),true,'ETCH/BEETLE V1.4 allowed');
for(const i of ['FM-1_014','FM-1_000','FM-1_2070','FM-2_015','ota-FM-1_999','FM-1_999999','FM-1_abc'])assert.equal(a.isSupportedEtchIdentity(i),false,'unknown firmware refused '+i);
assert.equal(a.firmwareRoute('FM-1_015'),'OFFICIAL V15 -> BEETLE');
assert.equal(a.firmwareRoute('FM-1_014'),null);
// Exact embedded binary, not a guessed file or hash.
const firmwareBlock=html.match(/<script id="firmware-embedded" type="application\/octet-stream">\s*([\s\S]*?)\s*<\/script>/);
assert(firmwareBlock,'embedded fwsc exists');
const fw=new Uint8Array(Buffer.from(firmwareBlock[1].replace(/\s+/g,''),'base64'));
const fwExternal=new Uint8Array(fs.readFileSync(path.resolve(__dirname,'../../../BEETLE-V0.69-STORAGE-EXPERIMENTAL.fwsc')));
assert(Buffer.from(fw).equals(Buffer.from(fwExternal)),'same protected firmware as V0.64 Step Groove release');
const hash=crypto.createHash('sha256').update(fw).digest('hex');
assert.equal(hash,a.C.HASH_ET);
(async()=>{
  const pkg=await a.verifyPackage(fw,'etch');
  assert.equal(pkg.product,'FM-1_2069');
  for(const originalId of ['FM-1_015','FM-1_2042','ota-FM-1_015']){
    const events=[],messages=[];
    const mkLink=(name)=>({send:()=>{events.push('send:'+name);return true},close:async()=>{events.push('close:'+name)}});
    const fake=new a.Engine(null,(s)=>messages.push(s),(s)=>events.push('stage:'+s));
    let waitCount=0;
    fake.waitFor=async predicate=>{
      waitCount++;
      const step = originalId==='ota-FM-1_015' ? 2 : waitCount;
      const dev=step===1?{id:{text:'ota-FM-1_015',loader:true},link:mkLink('loader')}
        :{id:{text:'FM-1_2069',loader:false},link:mkLink('booted')};
      assert(predicate(dev.id),'appropriate loader / boot identity queried');
      return dev;
    };
    fake.serve=async (link,image,finish,idle,progress)=>{
      events.push('serve:'+finish.toString(16));
      assert.equal(image.length,pkg.image.length);
      assert(finish===a.C.FINISH_CHECK||finish===a.C.FINISH_WRITE);
      progress(30,image.length);
      return 30;
    };
    await fake.install({id:{text:originalId,loader:originalId==='ota-FM-1_015'},link:mkLink('initial')},pkg);
    assert(events.includes('serve:'+a.C.FINISH_WRITE.toString(16)));
    assert.equal(events.some(x=>x==='serve:'+a.C.FINISH_CHECK.toString(16)),originalId!=='ota-FM-1_015');
    assert(events.includes('stage:done'),'finished only after expected boot identity');
    console.log('PASS simulated OTA source',originalId,'stages',events.filter(e=>e.startsWith('stage:')).join(','));
  }
  // Reject unknown before sending UPGRADE, not simply at the UI layer.
  let sent=false;
  const f=new a.Engine(null,()=>{},()=>{});
  await assert.rejects(()=>f.install({id:{text:'FM-1_014',loader:false},link:{send:()=>{sent=true;return true}}},pkg),/Unsupported source/);
  assert(!sent,'no command sent to unverified firmware');
  const bad=new Uint8Array(fw);bad[200]^=0x01;
  await assert.rejects(()=>a.verifyPackage(bad,'etch'),/hash does not match/);
  // Defensive negative: installing a different product cannot pass.
  await assert.rejects(()=>f.install({id:{text:'FM-1_015',loader:false},link:{send:()=>{sent=true;return true}}},{product:'FM-2_2049',image:pkg.image}),/Wrong device/);
  console.log('PASS integrity',hash,'bytes',fw.length,'unknown rejected, modified firmware rejected');
})().catch(e=>{console.error(e.stack);process.exitCode=1;});
