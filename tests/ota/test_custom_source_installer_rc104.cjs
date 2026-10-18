'use strict';
// Browser WebInstaller's embedded OTA core executed in a controlled Node sandbox.
const fs=require('fs'), path=require('path'), vm=require('vm'),assert=require('node:assert/strict'),crypto=require('crypto');
const root=path.resolve(__dirname,'../../..');
const html=fs.readFileSync(path.join(root,'WebInstaller.html'),'utf8');
const match=html.match(/<script>(\/\* SPDX-License-Identifier: GPL-3\.0-only[\s\S]*?)<\/script>/);
assert(match,'WebInstaller must embed the full OTA implementation');
const context={module:{exports:{}},Uint8Array,ArrayBuffer,Set,Date,Math,Number,String,Error,Promise,console,crypto:crypto.webcrypto,atob:globalThis.atob,
  setTimeout:callback=>{callback();return 1;}};
context.globalThis=context;
vm.runInNewContext(match[1],context,{filename:'installer-rc104-custom-source.js'});
const api=context.module.exports;
assert.equal(api.isSupportedEtchIdentity('FM-1_7777'),false,'unknown version must not be falsely described as previously verified');
for(const id of ['FM-1_2033','FM-1_1037','FM-1_015']) {
 assert.equal(api.sourceCompatibility(id,'etch'),'verified',id);
}
for(const id of ['FM-1_1036','FM-1_7777','FM-1_555','FM-1_2026','FM-1_99999','FM-1_999999']) {
 assert.equal(api.isCustomFM1Source(id),true,id);
 assert.equal(api.sourceCompatibility(id,'etch'),'experimental',id);
 assert.match(api.firmwareRoute(id),/PROTOCOL CHECK REQUIRED/);
 assert.equal(api.sourceCompatibility(id,'stock'),'unsupported','stock route must not be silently broadened');
 assert.equal(api.sourceCompatibility(id,'freedom'),'unsupported','switching to unrelated firmware stays restricted');
}
for(const id of ['FM-2_1234','X-FM-1_100','ota-FM-1_900','FM-1_', 'FM-1_-1','FM-1_1evil','FM-1_'+ '9'.repeat(30)]){
 assert.equal(api.sourceCompatibility(id,'etch'),'unsupported',id);
}
assert.equal(api.sourceCompatibility('ota-FM-1_015','etch'),'verified','recognized loader resume still supported');
assert(html.includes('id="custom-source-notice"'));
assert(html.includes('The identity query succeeded, but its update protocol has NOT been verified'));
const firmware=fs.readFileSync(path.join(root,'BEETLE-1.01-RC1.0.4-EXPERIMENTAL.fwsc'));
const embedded=html.match(/<script id="firmware-embedded" type="application\/octet-stream">\s*([\s\S]*?)\s*<\/script>/);
assert(embedded,'actual firmware embedded');
assert(Buffer.from(embedded[1].replace(/\s/g,''),'base64').equals(firmware),'firmware must remain identical');
const sha=crypto.createHash('sha256').update(firmware).digest('hex');
assert.equal(api.C.HASH_ET,sha,'verified firmware hash must stay pinned');
(async()=>{
 const pkg=await api.verifyPackage(new Uint8Array(firmware),'etch');
 assert.equal(pkg.product,'FM-1_2101');
 // A custom source with a compatible CHECK -> FM1 loader -> WRITE -> verified reboot.
 const phases=[];
 const mkLink=name=>({send:()=>{phases.push('send:'+name);return true;},close:async()=>{phases.push('close:'+name);}});
 const engine=new api.Engine(null,x=>phases.push('log:'+x),x=>phases.push('phase:'+x));
 let reconnects=0;
 engine.waitFor=async pred=>{
   reconnects++;
   const id=reconnects===1?{text:'ota-FM-1_015',loader:true}:{text:pkg.product,loader:false};
   assert(pred(id),'updater should only accept the expected FM1 stage identity');
   return {id,link:mkLink('reconnect'+reconnects)};
 };
 engine.serve=async (_link,_img,finish)=>{phases.push('serve:'+finish.toString(16));return 8;};
 await engine.install({id:{text:'FM-1_7777',loader:false},link:mkLink('custom-source')},pkg);
 assert(phases.indexOf('serve:e0000000')!==-1);
 assert(phases.indexOf('serve:f0000000')>phases.indexOf('serve:e0000000'));
 assert(phases.some(x=>x.includes('phase:done')));
 // A custom source that fails CHECK MUST NOT get any WRITE data.
 const noWrite=[];
 const failEngine=new api.Engine(null,()=>{},()=>{});
 failEngine.serve=async (_link,_img,finish)=>{
   noWrite.push(finish);
   throw new Error('CHECK rejected');
 };
 await assert.rejects(()=>failEngine.install({id:{text:'FM-1_8765',loader:false},link:mkLink('failed-source')},pkg),/CHECK rejected/);
 assert.deepEqual(noWrite,[api.C.FINISH_CHECK]);
 // Even after a CHECK, a nonstandard/unrecognized loader may not be used for an unlisted source.
 const badLoaderEngine=new api.Engine(null,()=>{},()=>{});
 const badLoaderPhases=[];
 badLoaderEngine.serve=async (_link,_img,finish)=>{badLoaderPhases.push(finish);return 8;};
 badLoaderEngine.waitFor=async predicate=>{
   assert.equal(predicate({text:'ota-FM-1_9999',loader:true}),false);
   assert.equal(predicate({text:'ota-FM-2_015',loader:true}),false);
   return null;
 };
 await assert.rejects(()=>badLoaderEngine.install({id:{text:'FM-1_7777',loader:false},link:mkLink('bad-loader')},pkg),/Update loader did not reconnect/);
 assert.deepEqual(badLoaderPhases,[api.C.FINISH_CHECK],'unrecognized loader must not be flashed');
 // An unknown source cannot switch to arbitrary stock or another custom image.
 let sent=false;
 const link={send:()=>{sent=true;return true;},close:async()=>{}};
 await assert.rejects(()=>new api.Engine(null,()=>{},()=>{}).install({id:{text:'FM-1_7777',loader:false},link},{...pkg,kind:'stock'}),/Unsupported FM1 source/);
 assert.equal(sent,false);
 // An unrecognized loader (ota-FM-1_9876) is not accepted as an initial source.
 await assert.rejects(()=>new api.Engine(null,()=>{},()=>{}).install({id:{text:'ota-FM-1_9876',loader:true},link},pkg),/Unsupported FM1 source/);
 assert.equal(sent,false);
 console.log('PASS: unlisted FM1 custom IDs enter compatibility mode; known and stock routes stay unchanged');
 console.log('PASS: successful CHECK -> known loader -> WRITE -> reboot; failed CHECK never reaches WRITE');
 console.log('PASS: firmware remains unchanged and SHA-256-pinned, unsafe source/model/loader rejected');
})().catch(e=>{console.error(e);process.exitCode=1});
