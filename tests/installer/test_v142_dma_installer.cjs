// SPDX-License-Identifier: GPL-3.0-only
// Node-side verification of the complete pinned package; no device or USB access.
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const root=path.resolve(__dirname,'../..');
const h=fs.readFileSync(path.join(root,'WebInstaller.html'),'utf8');
const bytes=fs.readFileSync(path.join(root,'BEETLE-V1.4.5-PLAYHEAD-GHOST-FIX-UNVERIFIED.fwsc'));
const embedded=h.match(/<script id="firmware-embedded" type="application\/octet-stream">\s*([A-Za-z0-9+/=\s]+?)\s*<\/script>/);
assert(embedded);
assert.deepEqual(Buffer.from(embedded[1].replace(/\s/g,''),'base64'),bytes);
const scripts=[...h.matchAll(/<script(?: [^>]*)?>([\s\S]*?)<\/script>/g)];
const script=scripts.find(x=>x[1].includes('root.ETCHOta=api;'))?.[1];
assert(script,'Missing OTA script');
const rootScope={
 module:{exports:{}},crypto:require('node:crypto').webcrypto,
 atob:x=>Buffer.from(x,'base64').toString('binary'),
 Uint8Array,ArrayBuffer,TextDecoder,TextEncoder,
 setTimeout,clearTimeout,console
};
vm.createContext(rootScope);vm.runInContext(script,rootScope,{timeout:10000});
const api=rootScope.module.exports;
assert.equal(api.C.ID_ET,'FM-1_2123');
assert.equal(api.C.ET_LENGTH,610086);
assert.equal(api.C.HASH_ET,require('node:crypto').createHash('sha256').update(bytes).digest('hex'));
(async()=>{
 const valid=await api.verifyPackage(new Uint8Array(bytes),'etch');
 assert.equal(valid.product,'FM-1_2123');
 assert.equal(valid.hash,api.C.HASH_ET);
 // A single tampered app byte is rejected before even reaching loader logic.
 const tampered=Buffer.from(bytes);tampered[Math.floor(tampered.length/2)]^=1;
 await assert.rejects(api.verifyPackage(new Uint8Array(tampered),'etch'),/hash does not match/i);
 // Old 1.3.3 image and wrong product ID must not silently pass as current.
 const old=fs.readFileSync(path.join(root,'BEETLE-V1.3.3-USB-MULTITRACK-EXPERIMENTAL.fwsc'));
 await assert.rejects(api.verifyPackage(new Uint8Array(old),'etch'),/hash does not match/i);
 assert.equal(api.firmwareRoute('FM-1_FAKE'),null);
 assert.equal(api.firmwareRoute('FM-1_015')!==null,true);
 // Simulated transport: successful CHECK -> approved OTA loader -> WRITE -> reboot.
 const stages=[];
 const link=(label)=>({send:()=>{stages.push('send:'+label);return true},close:async()=>{stages.push('close:'+label)}});
 const engine=new api.Engine(null,x=>stages.push('log:'+x),x=>stages.push('phase:'+x));
 let reconnects=0;
 engine.waitFor=async predicate=>{
  reconnects++;
  const id=reconnects===1?{text:'ota-FM-1_015',loader:true}:{text:'FM-1_2123',loader:false};
  assert.equal(predicate(id),true);
  return {id,link:link('reconnect'+reconnects)};
 };
 engine.serve=async (_l,_image,finish)=>{stages.push('serve:'+finish.toString(16));return 8;};
 await engine.install({id:{text:'FM-1_015',loader:false},link:link('source')},valid);
 assert(stages.includes('serve:e0000000'));
 assert(stages.includes('serve:f0000000'));
 assert(stages.some(x=>x.includes('phase:done')));
 // CHECK denial cannot reach WRITE, even with a plausible image.
 const denied=[];
 const failure=new api.Engine(null,()=>{},()=>{});
 failure.serve=async (_l,_img,finish)=>{denied.push(finish);throw Error('CHECK refused');};
 await assert.rejects(failure.install({id:{text:'FM-1_015',loader:false},link:link('denied')},valid),/CHECK refused/);
 assert.deepEqual(denied,[api.C.FINISH_CHECK]);
 // Incorrect OTA loader cannot receive write stage.
 const badLoader=[];
 const rejectLoader=new api.Engine(null,()=>{},()=>{});
 rejectLoader.serve=async (_l,_img,finish)=>{badLoader.push(finish);return 1;};
 rejectLoader.waitFor=async predicate=>{
  assert.equal(predicate({text:'ota-FM-1_9999',loader:true}),false);
  return null;
 };
 await assert.rejects(rejectLoader.install({id:{text:'FM-1_015',loader:false},link:link('invalid-loader')},valid),/no compatible OTA loader|no compatible OTA loader reconnected/i);
 assert.deepEqual(badLoader,[api.C.FINISH_CHECK]);
 console.log('PASS: actual JS validates current firmware; old, tampered, unknown sources rejected; USB untouched');
})().catch(e=>{console.error(e);process.exitCode=1;});
