// Browser-unit simulation for BEETLE sample auto conversion and BD4S handoff.
const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const src=fs.readFileSync(path.join(__dirname,'../../BEETLE-Project-Manager.html'),'utf8');
const match=src.match(/const BeetleSampleConvert=\(\(\)=>\{([\s\S]*?)\n\}\)\(\);/);
assert(match,'converter module missing');
const fun='const BeetleSampleConvert=(()=>{'+match[1]+'\n})();';
const fakeChannels=[Float32Array.from({length:48000},(_,i)=>Math.sin(i/32)*0.75),Float32Array.from({length:48000},(_,i)=>Math.sin(i/32)*0.25)];
let closed=0,decodeCalls=0,renderCalls=0;
class AudioContext{
 async decodeAudioData(buf){decodeCalls++;assert(buf.byteLength>0);return {length:48000,sampleRate:48000,duration:1,numberOfChannels:2,getChannelData:i=>fakeChannels[i]};}
 async close(){closed++}
}
class OfflineAudioContext{
 constructor(channels,frames,rate){assert(channels===1&&rate===22050&&frames===22050);this.length=frames;this.source=null;}
 createBuffer(c,n,rate){assert(c===1&&n===48000&&rate===48000);const data=new Float32Array(n);return {getChannelData:()=>data,data};}
 createBufferSource(){const src={buffer:null,connect:()=>{},start:()=>{}};this.source=src;return src;}
 async startRendering(){renderCalls++;assert(this.source.buffer.data.some(v=>v!==0));const input=this.source.buffer.data;
  return {getChannelData:()=>Float32Array.from({length:this.length},(_,i)=>input[Math.floor(i*48000/22050)])};}
}
const sandbox={window:{AudioContext,OfflineAudioContext},Uint8Array,DataView,Math,Error,Uint16Array,Float32Array,BeetleGlobalBank:{wavInfo:()=>{throw Error('not PCM WAV')}}};
vm.createContext(sandbox);vm.runInContext(fun+'this.convert=BeetleSampleConvert;',sandbox);
(async()=>{
 const res=await sandbox.convert.normalizeBytes(new Uint8Array([77,80,51,1,0,0,0,0,0,0,0,0,0,0,0,0]));
 assert(res.converted&&!res.trimmed);assert(res.data.length===44+22050*2);
 const view=new DataView(res.data.buffer);
 assert(view.getUint16(20,true)===1&&view.getUint16(22,true)===1&&view.getUint32(24,true)===22050&&view.getUint16(34,true)===16);
 assert(decodeCalls===1&&closed===1&&renderCalls===1);
 sandbox.BeetleGlobalBank.wavInfo=()=>({format:'PCM',channels:1,bitsPerSample:16,sampleRate:22050,frames:22050});
 const codec=src.match(/const BeetleDeviceCodec=\(\(\)=>\{([\s\S]*?)\n\}\)\(\);/);assert(codec,'BD4S codec missing');
 sandbox.crc32=()=>123456;
 vm.runInContext('const BeetleDeviceCodec=(()=>{'+codec[1]+'\n})();this.codec=BeetleDeviceCodec;',sandbox);
 const bd4=sandbox.codec.encode(res.data);
 assert(String.fromCharCode(...bd4.subarray(0,4))==='BD4S' && new DataView(bd4.buffer).getUint32(8,true)===22050);
 const kept=await sandbox.convert.normalizeBytes(res.data);
 assert(!kept.converted&&kept.data===res.data&&decodeCalls===1);
 console.log('PASS: stereo 48k audio normalized to 22.05kHz mono PCM16 and compliant WAV bypasses conversion');
})().catch(e=>{console.error(e);process.exit(1)});
