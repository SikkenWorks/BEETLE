'use strict';
const fs=require('node:fs'),assert=require('node:assert/strict'),vm=require('node:vm');
const {webcrypto}=require('node:crypto');
const source=fs.readFileSync(require('node:path').join(__dirname,'../../web/BEETLE-Project-Manager.html'),'utf8');
const js=source.match(/<script>([\s\S]+)<\/script>/)[1].split("$('connect').onclick=")[0];
const context=vm.createContext({Uint8Array,TextEncoder,Blob,crypto:webcrypto,console,setTimeout,clearTimeout,document:{addEventListener:()=>{}},navigator:{},isSecureContext:true});
vm.runInContext(js,context);
async function run(){
 const payload=Uint8Array.from({length:105},(_,i)=>(i*29+123)&255);
 const get=(s)=>vm.runInContext(s,context);
 const pack=get('pack7'),unpack=get('unpack7'),makeFrame=get('frame'),parse=get('parseFrame');
 assert.deepEqual([...unpack(pack(payload))],[...payload]);
 const frame=makeFrame(0x56,[17,0,9,24,...payload.slice(0,24)]);
 const result=parse(frame);assert.equal(result.command,0x56);assert.equal(result.body[0],17);assert.equal(result.body[3],24);
 const tamperedFrame=Uint8Array.from(frame);tamperedFrame[tamperedFrame.length-2]^=1;assert.equal(parse(tamperedFrame),null);
 const enc=get('encryptProject'),dec=get('decryptProject');
 const fileA=await enc(payload);
 const fileB=await enc(payload);
 const fileABytes=new Uint8Array(await fileA.arrayBuffer());
 assert.equal(fileABytes.length,21+payload.length+16);
 assert.equal(Buffer.from(fileABytes.slice(0,8)).toString('ascii'),'BEETLE01');
 assert.equal(fileABytes[8],2);
 assert.deepEqual([...await dec(fileA)],[...payload]);
 assert.notDeepEqual(new Uint8Array(await fileB.arrayBuffer()),fileABytes,'GCM nonce should be random per export');
 const tamperedFile=Uint8Array.from(fileABytes);tamperedFile[30]^=1;
 await assert.rejects(()=>dec(new Blob([tamperedFile])),/damaged|integrity/);
 await assert.rejects(()=>dec(new Blob([new Uint8Array(8)])),/Invalid .*file size/);
 // Legacy V1 can still be decrypted using its original user-supplied password.
 const pw='Test-backup-passphrase-2026', salt=webcrypto.getRandomValues(new Uint8Array(16)),iv=webcrypto.getRandomValues(new Uint8Array(12));
 const header=Uint8Array.from([...new TextEncoder().encode('BEETLE01'),1,...salt,...iv]);
 const k=await get('makeKey')(pw,salt,'encrypt');
 const ciphertext=new Uint8Array(await webcrypto.subtle.encrypt({name:'AES-GCM',iv,additionalData:header,tagLength:128},k,payload));
 const v1=new Blob([header,ciphertext]);
 const legacy=get('decryptLegacyV1');
 assert.deepEqual([...await legacy(v1,pw)],[...payload]);
 await assert.rejects(()=>legacy(v1,'wrong password'),/Incorrect legacy password/);
 console.log('PASS MIDI frame + checksums, AES-256-GCM V2 no-password roundtrip + randomness + tamper refusal, legacy V1 passwords');
}
run().catch(e=>{console.error(e);process.exitCode=1;});
