/* Native FM1 TRK1 V12-V17 -> browser simulator model. Based on track_project_store.c.
   Rejects unknown or truncated data, and validates both native CRC32 fields. */
const BeetleNative=(()=>{
 const crc=b=>{let c=0xffffffff;for(const v of b){c^=v;for(let i=0;i<8;i++)c=(c>>>1)^((c&1)?0xedb88320:0);}return (~c)>>>0;};
 const u16=(b,i)=>b[i]|(b[i+1]<<8), u32=(b,i)=>(b[i]|b[i+1]<<8|b[i+2]<<16|b[i+3]<<24)>>>0;
 const ascii=a=>String.fromCharCode(...a).replace(/\0/g,'').trim();
 function decode(raw){
  const b=raw instanceof Uint8Array?raw:new Uint8Array(raw);
  if(b.length<42||b.length>24576||ascii(b.subarray(0,4))!=='TRK1')throw Error('Not a native FM1 TRK1 project.');
  const schema=u16(b,4),slot=u16(b,6);
  if(schema<12||schema>17)throw Error('Native project schema V'+schema+' is not supported by this simulator importer (V12–V17).');
  if(slot>=12||u32(b,8)!==b.length||u32(b,12)!==crc(b.subarray(24))||u32(b,16)!==crc(b.subarray(0,16)))throw Error('Native project CRC, length, or slot is invalid.');
  let pos=24;
  const need=n=>{if(pos+n>b.length)throw Error('Truncated native project.');};
  const read=()=>{need(1);return b[pos++];};
  const take=n=>{need(n);const v=b.subarray(pos,pos+n);pos+=n;return v;};
  const word=()=>{const v=take(2);return u16(v,0);};
  const signed=x=>x>127?x-256:x;
  const inRange=(n,max,label)=>{if(n>max)throw Error('Bad '+label+' in native project.');return n;};
  const name=ascii(take(12)),innerSlot=read();if(innerSlot!==slot)throw Error('Native project slot mismatch.');
  const root=read(),mode=read(),keylock=read(),bpm=word(),count=read(),songLoop=read(),songMode=read();
  if(root>=12||mode>=9||keylock>2||bpm<30||bpm>300||count<1||count>64||songLoop>1||songMode>1)throw Error('Invalid native project settings.');
  const p=freshProject(name);p.bpm=bpm;p.harmony={root,mode,keylock};p.song=[];p.songLoop=!!songLoop;p.nativeSlot=slot;p.nativeSchema=schema;
  for(let i=0;i<count;i++){const pattern=read(),repeats=read();if(pattern>=48||repeats<1||repeats>16)throw Error('Invalid Song entry.');p.song.push({pattern,repeats});}
  for(let c=0;c<6;c++){const volume=read(),pan=signed(read()),mute=read(),solo=read(),swing=read();if(volume>120||Math.abs(pan)>100||mute>1||solo>1||swing>100)throw Error('Invalid mixer lane.');p.mixer[c]={volume:volume/100,pan:pan/100,mute:!!mute,solo:!!solo,swing};}
  for(let r=0;r<48;r++){
   const len=read();if(len<1||len>16)throw Error('Invalid pattern length.');
   const mask=take(12),count=Array.from(mask).reduce((n,x)=>n+x.toString(2).replace(/0/g,'').length,0),bitsCount=schema>=12?40:33,packed=take(Math.ceil(count*bitsCount/8));
   let bit=0;
   const readBits=n=>{let v=0;for(let j=0;j<n;j++){v|=((packed[bit>>>3]>>>(bit&7))&1)<<j;bit++;}return v;};
   let pat=null;
   for(let c=0;c<6;c++)for(let step=0;step<16;step++){
    const maskBit=c*16+step;if(!(mask[maskBit>>>3]&(1<<(maskBit&7))))continue;
    const note=readBits(8),inst=readBits(2),chd=readBits(4),fx=readBits(4),amount=readBits(7),dyn=readBits(8),groove=schema>=12?readBits(7):0;
    if(fx>=12||amount>100||(c===5?!(note===255||note===254||note<14):!(note===255||note===254||note<128)))throw Error('Invalid native step.');
    if(!pat)pat=emptyPattern();pat.channels[c][step]={note:note===255?null:note,inst,chd,fx,amount,dyn,groove};
   }
   const events=read();if(events>64)throw Error('Invalid Punch-In event count.');
   for(let e=0;e<events;e++){
    const value=read(),aux=read(),row=value&15,key=(value>>>4)&7;
    if(!pat)pat=emptyPattern();
    const obj={row,key,down:!!(value&128)};
    if(aux&128){obj.down=true;obj.param=aux&127;obj.paramChange=true;}
    else if(aux!==127)obj.param=aux;
    pat.punch.push(obj);
   }
   if(pat||len!==16){pat??=emptyPattern();pat.length=len;p.patterns[r]=pat;}
  }
  const instRead=()=>{
    const type=read(),attack=read(),decay=read(),sustain=read(),release=read(),duty=read(),volume=read(),cutoff=word();
    const waveA=read(),waveB=read(),coarse=signed(read()),fine=signed(read()),detune=read(),mix=read(),pulseWidth=read(),morph=read();
    const coarseB=signed(read()),fineB=signed(read()),pulseWidthB=read(),drive=read(),sub=read(),noise=read(),fmDepth=read();
    if(type>=12||[attack,decay,sustain,release,duty,volume].some(n=>n>100))throw Error('Invalid native instrument.');
    return {type,attack:attack/100,decay:decay/100,sustain:sustain/100,release:release/100,duty:duty/100,volume:volume/100,cutoff,waveA,waveB,coarse,fine,detune,mix,pulseWidth,morph,coarseB,fineB,pulseWidthB,drive,sub,noise,fmDepth,lfoRate:5,lfoDepth:0};
  };
  for(let i=0;i<4;i++)p.instruments[i]=instRead();
  p.drumComponents=[];
  for(let i=0;i<14;i++){const att=read(),dec=read(),sus=read(),rel=read(),low=read(),mid=read(),high=read(),gain=read(),pitch=signed(read());p.drumComponents.push([att,dec,sus,rel,pitch,low,mid,high,gain]);}
  const master=p.master;for(let k=0;k<4;k++)master['eq'+k]=read();
  master.eq_freq=Array.from({length:4},read);master.eq_width=Array.from({length:4},read);
  for(const k of ['comp_threshold','comp_ratio','comp_gain','reverb_wet','reverb_feedback','reverb_tone'])master[k]=read();
  master.enabled=Array.from({length:3},()=>!!read());master.order=Array.from({length:3},read);
  p.punchParams=Array.from({length:5},read);
  if(schema>=10){master.comp_attack=read();master.reverb_spread=read();}
  if(schema>=13)p.quantizeMutes=!!read();
  if(schema>=14){const mask=read();p.snapshots=Array.from({length:4},(_,i)=>{const v=read();return mask&(1<<i)?v:null;});}
  if(schema>=15)p.muteGroups=Array.from({length:4},read);
  if(schema>=16){const valid=read();p.library=Array.from({length:8},(_,i)=>valid&(1<<i)?{name:ascii(take(8)),patch:instRead()}:null);}
  if(schema>=17){p.typeBanks=Array.from({length:4},()=>({}));for(let i=0;i<4;i++){const mask=word();if(mask&~((1<<12)-1))throw Error('Unknown native instrument type.');for(let t=0;t<12;t++)if(mask&(1<<t)){const patch=instRead();if(patch.type!==t)throw Error('Corrupted instrument bank.');p.typeBanks[i][t]=patch;}}}
  if(pos!==b.length)throw Error('Native project has unexpected trailing bytes.');
  beetleVerifyProjectShape(p);
  return p;
 }
 // Serialize the full editor model into the firmware V17 TRK1 wire format.
 // No flash activity; the Project Manager handles all subsequent FM1 safety gates.
 function encode(p){
  beetleVerifyProjectShape(p);
  const out=new Uint8Array(12256);let i=24;
  const byte=n=>{if(!Number.isInteger(n)||n<0||n>255)throw Error('Native byte field out of range.');if(i>=out.length)throw Error('FM1 project exceeds 12 KiB.');out[i++]=n;};
  const b16=n=>{byte(n&255);byte((n>>>8)&255);};
  const pct=n=>Math.round(n*100);
  const u8=n=>Math.round(n||0);
  const limited=(v,min,max,name)=>{if(!Number.isInteger(v)||v<min||v>max)throw Error('Invalid '+name+' for native FM1 export: '+v);return v;};
  const bSigned=n=>limited(u8(n),-128,127,'signed byte')&255;
  const bByte=(n,lo=0,hi=100)=>byte(limited(u8(n),lo,hi,'parameter'));
  const name=String(p.name||'UNTITLED').toUpperCase().replace(/[^A-Z0-9 _-]/g,' ').slice(0,12).padEnd(12,' ');
  for(const c of name)byte(c.charCodeAt(0));
  const slot=limited(Number.isInteger(p.nativeSlot)?p.nativeSlot:0,0,11,'project slot');
  byte(slot);byte(limited(p.harmony?.root??0,0,11,'root'));byte(limited(p.harmony?.mode??0,0,8,'scale'));byte(limited(p.harmony?.keylock??0,0,2,'keylock'));
  b16(limited(u8(p.bpm),30,300,'BPM'));byte(p.song.length);byte(p.songLoop===false?0:1);byte(p.nativeSongEnabled?1:0);
  for(const entry of p.song){byte(limited(entry.pattern,0,47,'song pattern'));byte(limited(entry.repeats,1,16,'song repeats'));}
  for(const [ch,m] of p.mixer.entries()){
   bByte(pct(m.volume),0,120);byte(bSigned(pct(m.pan)));byte(m.mute?1:0);byte(m.solo?1:0);bByte(m.swing??0);
  }
  for(let r=0;r<48;r++){
   const pat=p.patterns[r]||emptyPattern();if(!p.patterns[r])for(const st of pat.channels[5])st.inst=3;byte(limited(pat.length,1,16,'pattern length'));
   const maskAt=i;for(let j=0;j<12;j++)byte(0);
   const selected=[];
   for(let ch=0;ch<6;ch++)for(let step=0;step<16;step++){
    const s=pat.channels[ch][step],note=s.note==null?255:limited(s.note,0,255,'note');
    const inst=limited(s.inst,0,3,'instrument'),chd=limited(s.chd,0,15,'chord'),fx=limited(s.fx,0,11,'FX');
    const amount=limited(u8(s.amount),0,100,'effect value'),dyn=limited(s.dyn??0,0,255,'dynamics'),groove=limited(s.groove??0,0,127,'groove');
    if(ch===5 && note!==254 && note!==255 && note>=14)throw Error('Invalid drum note');
    if(ch<5 && note!==254 && note!==255 && note>=128)throw Error('Invalid synth note');
    if(note===255&&inst===(ch===5?3:ch%4)&&chd===0&&fx===0&&amount===50&&dyn===0&&groove===0)continue;
    const index=ch*16+step;out[maskAt+(index>>>3)]|=1<<(index&7);
    selected.push([note,inst,chd,fx,amount,dyn,groove]);
   }
   // C pack_bits is little-endian at the individual bit level.
   let bits=0,val=0;
   const put=(n,count)=>{for(let j=0;j<count;j++){val|=((n>>>j)&1)<<bits;if(++bits===8){byte(val);bits=0;val=0;}}};
   for(const s of selected){put(s[0],8);put(s[1],2);put(s[2],4);put(s[3],4);put(s[4],7);put(s[5],8);put(s[6],7);}
   if(bits)byte(val);
   const events=pat.punch||[];
   if(events.length>24)throw Error('FM1 only supports 24 Punch-In events per pattern.');
   byte(events.length);
   for(const ev of events){
    const row=limited(u8(ev.row),0,15,'punch row'),key=limited(u8(ev.key),0,7,'punch key');
    let param=ev.param===undefined?255:limited(u8(ev.param),0,100,'punch parameter');
    const change=ev.paramChange===true;
    byte(row|(key<<4)|(ev.down?128:0));
    byte(change?(param===255?50:param)|128:(param===255?127:param));
   }
  }
  const instr=v=>{
   if(!v)throw Error('A native instrument patch is missing.');
   byte(limited(u8(v.type),0,11,'instrument type'));
   for(const k of ['attack','decay','sustain','release'])bByte(pct(v[k]??0));
   bByte(pct(v.duty??0.5));bByte(pct(v.volume??0.65));b16(limited(u8(v.cutoff??7000),0,65535,'cutoff'));
   bByte(v.waveA??0,0,4);bByte(v.waveB??1,0,4);
   byte(bSigned(v.coarse??0));byte(bSigned(v.fine??0));
   bByte(v.detune??12);bByte(v.mix??50);bByte(v.pulseWidth??50,5,95);bByte(v.morph??0);
   byte(bSigned(v.coarseB??v.coarse??0));byte(bSigned(v.fineB??v.fine??0));
   bByte(v.pulseWidthB??v.pulseWidth??50,5,95);
   for(const k of ['drive','sub','noise','fmDepth'])bByte(v[k]??0);
  };
  for(const v of p.instruments)instr(v);
  const drums=p.drumComponents||[];
  if(drums.length!==14)throw Error('Native FM1 export needs 14 drum components.');
  for(const d of drums){if(!Array.isArray(d)||d.length!==9)throw Error('Invalid drum component.');
   for(const k of [0,1,2,3,5,6,7,8])bByte(d[k]);byte(bSigned(limited(u8(d[4]),-36,36,'drum pitch')));
  }
  const m=p.master||{};
  for(let j=0;j<4;j++)bByte(m['eq'+j]??50);
  for(let j=0;j<4;j++)bByte(m.eq_freq?.[j]??50);
  for(let j=0;j<4;j++)bByte(m.eq_width?.[j]??50);
  for(const k of ['comp_threshold','comp_ratio','comp_gain','reverb_wet','reverb_feedback','reverb_tone'])bByte(m[k]??50);
  for(let j=0;j<3;j++)byte(m.enabled?.[j]===false?0:1);
  const order=m.order||[0,1,2];if([...(new Set(order))].length!==3)throw Error('Invalid MASTER effect order.');
  for(let j=0;j<3;j++)byte(limited(order[j],0,2,'MASTER effect order'));
  const punch=p.punchParams||[50,20,50,50,50];for(let j=0;j<5;j++)bByte(punch[j]??50);
  bByte(m.comp_attack??28);bByte(m.reverb_spread??50);
  byte(p.quantizeMutes?1:0);
  const snapshots=p.snapshots||[null,null,null,null];let valid=0;
  for(let j=0;j<4;j++)if(snapshots[j]!==null&&snapshots[j]!==undefined)valid|=1<<j;
  byte(valid);for(let j=0;j<4;j++)byte(limited(snapshots[j]??0,0,63,'snapshot mask'));
  for(let j=0;j<4;j++)byte(limited(p.muteGroups?.[j]??0,0,63,'mute group mask'));
  const lib=p.library||Array(8).fill(null);let libMask=0;
  for(let j=0;j<8;j++)if(lib[j])libMask|=1<<j;
  byte(libMask);
  for(let j=0;j<8;j++)if(libMask&(1<<j)){
   const label=String(lib[j].name||'').toUpperCase().replace(/[^A-Z0-9 _-]/g,' ').slice(0,8).padEnd(8,' ');
   for(const c of label)byte(c.charCodeAt(0));instr(lib[j].patch);
  }
  for(let j=0;j<4;j++){
   const bank=p.typeBanks?.[j]||{};
   let mask=0;
   for(let t=0;t<12;t++)if(bank[t])mask|=1<<t;
   b16(mask);
   for(let t=0;t<12;t++)if(mask&(1<<t)){
    if(bank[t].type!==t)throw Error('Instrument bank type mismatch.');instr(bank[t]);
   }
  }
  // The 24-byte inner header is sealed last, exactly like track_store_encode().
  const len=i;if(len>12256)throw Error('FM1 project does not fit in a 12 KiB slot.');
  const write16=(at,n)=>{out[at]=n&255;out[at+1]=(n>>>8)&255;};
  const write32=(at,n)=>{for(let k=0;k<4;k++)out[at+k]=(n>>>(8*k))&255;};
  out.set(Uint8Array.from([84,82,75,49]),0);write16(4,17);write16(6,slot);write32(8,len);
  write32(12,crc(out.subarray(24,len)));write32(16,crc(out.subarray(0,16)));
  return out.slice(0,len);
 }

 return {decode,encode,crc};
})();
