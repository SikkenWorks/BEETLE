'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const os=require('node:os');
const path=require('node:path');
const {spawnSync}=require('node:child_process');
const root=path.resolve(__dirname,'../..');
const Bank=require(path.join(root,'web/beetle-global-bank.js'));
const tool=path.join(__dirname,'inspect_native');
const temp=fs.mkdtempSync(path.join(os.tmpdir(),'beetle-bank-v146-'));
const asBytes=s=>Array.from(s,c=>c.charCodeAt(0));
function wave({frames=200,channels=1,rate=44100,bits=16,format=1,ext=false,list=false}={}){
 const align=channels*bits/8, dataBytes=frames*align,fmtLen=ext?40:16;
 const listLen=list?14:0; // LIST chunk: 8 bytes header + 6 bytes content
 const n=12+8+fmtLen+listLen+8+dataBytes;
 const a=new Uint8Array(n);const v=new DataView(a.buffer);
 a.set(asBytes('RIFF'),0);v.setUint32(4,n-8,true);a.set(asBytes('WAVEfmt '),8);v.setUint32(16,fmtLen,true);
 v.setUint16(20,ext?65534:format,true);v.setUint16(22,channels,true);v.setUint32(24,rate,true);
 v.setUint32(28,rate*align,true);v.setUint16(32,align,true);v.setUint16(34,bits,true);
 if(ext){v.setUint16(36,22,true);v.setUint16(38,bits,true);v.setUint32(40,0,true);
   v.setUint16(44,format,true);a.set([0,0,0,0,16,0,128,0,0,170,0,56,155,113],46);}
 let p=20+fmtLen;
 if(list){a.set(asBytes('LIST'),p);v.setUint32(p+4,6,true);a.set(asBytes('abcdef'),p+8);p+=14;}
 a.set(asBytes('data'),p);v.setUint32(p+4,dataBytes,true);p+=8;
 for(let i=0;i<dataBytes;i++)a[p+i]=i*17&255;
 return a;
}
function native(raw,shouldPass){
 const filename=path.join(temp,'check.beetlebank');fs.writeFileSync(filename,raw);
 const p=spawnSync(tool,[filename],{encoding:'utf8',timeout:15000});
 assert.equal(p.status,shouldPass?0:1,JSON.stringify({stdout:p.stdout,stderr:p.stderr}));return p.stdout;
}
try{
 assert.equal(Bank.crc32(Buffer.from('123456789')),0xcbf43926);
 const expectedEmpty=fs.readFileSync(path.join(root,'BEETLE-EMPTY-GLOBAL-LIBRARY.beetlebank'));
 assert.deepEqual(Buffer.from(Bank.encode(Bank.empty())),expectedEmpty,'v1 empty bank format must remain unchanged');
 native(expectedEmpty,true);
 for(const variant of [{},{channels:2,bits:24},{channels:2,bits:32,format:3},{channels:1,bits:64,format:3},{channels:2,bits:16,ext:true},{channels:1,bits:32,format:3,ext:true,list:true}]){
  const wav=wave(variant),info=Bank.wavInfo(wav);assert.equal(info.frames,variant.frames||200);
  assert.equal(info.format,(variant.format||1)===1?'PCM':'FLOAT');
  const bank=Bank.empty();bank[0]={name:'SLOT1',filename:'TEST.WAV',data:wav};bank[12]={name:'LAST',filename:'HI.WAV',data:wave({frames:150,bits:8})};
  const raw=Bank.encode(bank);native(raw,true);
  assert.deepEqual(Bank.decode(raw),bank);
  assert.equal(Bank.encode(Bank.decode(raw)).length,raw.length);
  assert.deepEqual(Buffer.from(Bank.encode(Bank.decode(raw))),Buffer.from(raw));
  const corrupt=raw.slice();corrupt[corrupt.length-20]^=0x80;assert.throws(()=>Bank.decode(corrupt));native(corrupt,false);
 }
 const f=wave();
 function reject(a){assert.equal(Bank.wavValid(a),false);assert.throws(()=>Bank.wavInfo(a));}
 const r=f.slice();r.set(asBytes('xxxx'),12);reject(r); // fake RIFF is no WAV
 const t=f.slice();new DataView(t.buffer).setUint32(40,100000,true);reject(t); // truncated data chunk
 const a=f.slice();new DataView(a.buffer).setUint16(32,5,true);reject(a); // invalid byte alignment
 const b=f.slice();new DataView(b.buffer).setUint16(20,6,true);reject(b); // compressed/unsupported
 const c=f.slice();c[8]=0;reject(c); // not WAVE
 const d=f.slice();new DataView(d.buffer).setUint32(4,3,true);reject(d); // invalid RIFF length
 const m=f.slice();new DataView(m.buffer).setUint32(40,399,true);reject(m); // partial PCM frame
 console.log('PASS: WAV format inspection + old-bank compatibility + JS/C v1 format interchange + corruption rejection');
}finally{fs.rmSync(temp,{recursive:true,force:true});}
