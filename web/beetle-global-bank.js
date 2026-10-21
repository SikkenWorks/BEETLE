/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE Global Sample Library — PC-side bank file, not FM1 flash storage.
 * Integrity-checked, no quality loss. Use independently of project files.
 */
(function(root,factory){
 const bank=factory();
 if(typeof module==='object'&&module.exports)module.exports=bank;
 else root.BeetleGlobalBank=bank;
})(typeof globalThis!=='undefined'?globalThis:this,function(){
 'use strict';
 const SLOTS=13, HEADER=24, ENTRY=40, MAX_FILE=33554432;
 const MAGIC=Uint8Array.from([66,69,69,84,66,65,78,75]); // BEETBANK
 const textEnc=typeof TextEncoder!=='undefined'?new TextEncoder():null;
 const textDec=typeof TextDecoder!=='undefined'?new TextDecoder():null;
 function crc32(data){let v=0xFFFFFFFF;for(const c of data){v^=c;for(let j=0;j<8;j++)v=(v>>>1)^((v&1)?0xEDB88320:0);}return (v^0xFFFFFFFF)>>>0;}
 function empty(){return Array.from({length:SLOTS},()=>({name:'',filename:'',data:new Uint8Array(0)}));}
 function u16(a,o){return a[o]|a[o+1]<<8;}
 function u32(a,o){return (a[o]|a[o+1]<<8|a[o+2]<<16|a[o+3]<<24)>>>0;}
 function p16(a,o,n){a[o]=n&255;a[o+1]=(n>>>8)&255;}
 function p32(a,o,n){a[o]=n&255;a[o+1]=n>>>8&255;a[o+2]=n>>>16&255;a[o+3]=n>>>24&255;}
 function utf8(s,max){const out=textEnc.encode(String(s||''));if(out.length>max)throw Error('Text field too long');return out;}
 function clean(entries){if(!Array.isArray(entries)||entries.length!==SLOTS)throw Error('Exactly 13 sample slots required');let size=HEADER+ENTRY*SLOTS;
  return entries.map((e,i)=>{const data=e.data instanceof Uint8Array?e.data:new Uint8Array(e.data||[]);
   const name=utf8(e.name||'',12),filename=utf8(e.filename||'',12);
   if(data.length&&!wavValid(data))throw Error('U'+(i+1)+': input is not a valid RIFF/WAVE file');
   if(!data.length&&(name.length||filename.length))throw Error('Empty sample has stray metadata');
   size+=data.length;if(size>MAX_FILE)throw Error('Library exceeds the 32 MiB PC-file limit');
   return {data,name,filename};});}
 function wavValid(a){if(a.length<44)return false;
  return a[0]===82&&a[1]===73&&a[2]===70&&a[3]===70&&a[8]===87&&a[9]===65&&a[10]===86&&a[11]===69&&u32(a,4)+8<=a.length;}
 function encode(entries){const items=clean(entries),size=HEADER+ENTRY*SLOTS+items.reduce((s,i)=>s+i.data.length,0);const out=new Uint8Array(size);out.set(MAGIC);
  p16(out,8,1);p16(out,10,SLOTS);p32(out,12,size);let at=HEADER+SLOTS*ENTRY;
  items.forEach((e,i)=>{const o=HEADER+i*ENTRY;p32(out,o,at);p32(out,o+4,e.data.length);p32(out,o+8,crc32(e.data));out[o+12]=e.name.length;out[o+13]=e.filename.length;
   out.set(e.name,o+14);out.set(e.filename,o+14+12);out.set(e.data,at);at+=e.data.length;});
  p32(out,16,crc32(out.subarray(HEADER)));p32(out,20,crc32(out.subarray(0,20)));return out;}
 function decode(raw){const a=raw instanceof Uint8Array?raw:new Uint8Array(raw);
  if(a.length<HEADER+SLOTS*ENTRY||a.length>MAX_FILE||!MAGIC.every((b,i)=>a[i]===b)||u16(a,8)!==1||u16(a,10)!==SLOTS||u32(a,12)!==a.length||u32(a,16)!==crc32(a.subarray(HEADER))||u32(a,20)!==crc32(a.subarray(0,20)))throw Error('Invalid or corrupted .beetlebank file');
  const entries=empty();let pos=HEADER+ENTRY*SLOTS;
  for(let i=0;i<SLOTS;i++){
   const off=HEADER+i*ENTRY,at=u32(a,off),len=u32(a,off+4),nameLen=a[off+12],fileLen=a[off+13];
   if(nameLen>12||fileLen>12||at!==pos||len>a.length-at||at>a.length||a[off+38]!==0||a[off+39]!==0)throw Error('Invalid bank index or overlapping sample data');
   const data=a.slice(at,at+len);if(u32(a,off+8)!==crc32(data)||(len&&!wavValid(data)))throw Error('Invalid WAV or sample checksum in slot '+(i+1));
   const name=textDec.decode(a.subarray(off+14,off+14+nameLen));const filename=textDec.decode(a.subarray(off+26,off+26+fileLen));
   if(!len&&(name||filename))throw Error('Invalid empty slot');
   entries[i]={name,filename,data};pos+=len;
  }
  if(pos!==a.length)throw Error('Trailing or unassigned bytes in bank file');
  return entries;}
 function used(entries){return entries.reduce((n,e)=>n+e.data.length,0);}
 return Object.freeze({SLOTS,MAX_FILE,empty,encode,decode,used,crc32,wavValid});
});
