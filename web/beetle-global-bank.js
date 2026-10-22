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
 // Table-based CRC is bit-for-bit compatible with the original format, but
 // avoids eight bit-iterations per byte for multi-megabyte WAV banks.
 const CRC_TABLE=new Uint32Array(256);
 for(let i=0;i<256;i++){let v=i;for(let bit=0;bit<8;bit++)v=(v>>>1)^((v&1)?0xEDB88320:0);CRC_TABLE[i]=v>>>0;}
 function crc32(data){let v=0xFFFFFFFF;for(let i=0;i<data.length;i++)v=CRC_TABLE[(v^data[i])&255]^(v>>>8);return (v^0xFFFFFFFF)>>>0;}
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
 function match(a,p,str){if(p<0||p+str.length>a.length)return false;for(let i=0;i<str.length;i++)if(a[p+i]!==str.charCodeAt(i))return false;return true;}
 /* Read-only strict WAV inspection. The bytes are NEVER transcoded or
  * resampled when stored in .beetlebank. Parsing validates RIFF chunk bounds,
  * audio frames and supported PCM/float formats (including WAVE Extensible).
  */
 function wavInfo(raw){
  const a=raw instanceof Uint8Array?raw:new Uint8Array(raw);
  if(a.length<44||!match(a,0,'RIFF')||!match(a,8,'WAVE'))throw Error('Expected RIFF/WAVE');
  const end=u32(a,4)+8;
  if(end<44||end>a.length)throw Error('Truncated or invalid RIFF length');
  let fmt=null,dataBytes=0,dataSeen=false,pos=12;
  while(pos<end){
   if(pos+8>end)throw Error('Truncated WAV chunk header');
   const len=u32(a,pos+4),payload=pos+8,after=payload+len;
   if(after>end||after+((len&1)>>>0)>end)throw Error('Truncated WAV chunk');
   if(match(a,pos,'fmt ')){
    if(fmt||len<16)throw Error('Duplicate or malformed WAV fmt chunk');
    const tag=u16(a,payload),channels=u16(a,payload+2),sampleRate=u32(a,payload+4);
    const bytesPerSec=u32(a,payload+8),align=u16(a,payload+12),bits=u16(a,payload+14);
    let formatTag=tag;
    if(tag===65534){
     if(len<40||u16(a,payload+16)<22||u16(a,payload+18)>bits)throw Error('Invalid WAV Extensible fmt');
     const guid=payload+24;
     // PCM and IEEE float canonical KSDATAFORMAT_SUBTYPE GUIDs.
     if(a[guid+2]!==0||a[guid+3]!==0||a[guid+4]!==0||a[guid+5]!==0||a[guid+6]!==16||a[guid+7]!==0||
       ![128,0,0,170,0,56,155,113].every((b,j)=>a[guid+8+j]===b))throw Error('Unsupported WAV Extensible subformat');
     formatTag=u16(a,guid);
    }
    if(formatTag!==1&&formatTag!==3)throw Error('Only PCM/IEEE-float WAV is supported');
    if(!channels||channels>32||sampleRate<1000||sampleRate>384000||
       !(formatTag===1?[8,16,24,32].includes(bits):[32,64].includes(bits))||
       align!==channels*(bits>>>3)||bytesPerSec!==sampleRate*align)throw Error('Invalid WAV channel/rate/bit-depth or block alignment');
    fmt={channels,sampleRate,bitsPerSample:bits,format:formatTag===1?'PCM':'FLOAT',blockAlign:align};
   }else if(match(a,pos,'data')){
    if(dataSeen)throw Error('Multiple WAV data chunks unsupported');
    dataSeen=true;dataBytes=len;
   }
   pos=after+(len&1);
  }
  if(!fmt||!dataSeen||!dataBytes||dataBytes%fmt.blockAlign!==0)throw Error('Missing WAV audio or incomplete audio frame');
  const frames=dataBytes/fmt.blockAlign;
  return Object.freeze({channels:fmt.channels,sampleRate:fmt.sampleRate,bitsPerSample:fmt.bitsPerSample,
   format:fmt.format,dataBytes,frames,durationSeconds:frames/fmt.sampleRate});
 }
 function wavValid(a){try{wavInfo(a);return true;}catch{return false;}}
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
 return Object.freeze({SLOTS,MAX_FILE,empty,encode,decode,used,crc32,wavValid,wavInfo});
});
