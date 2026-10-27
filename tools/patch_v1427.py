from pathlib import Path
import re
r=Path(__file__).resolve().parents[1]
chip=r/'firmware/fm1-native/track_chip.c'
hdr=r/'firmware/fm1-native/track_chip.h'
s=hdr.read_text(); old=' int32_t punch_filter_l,punch_filter_r;';assert old in s
s=s.replace(old,''' /* FX 10: per-channel sample-and-hold for REAL downsampling plus bit-depth
  * reduction. FX 15: click-safe high-pass + subtle hiss mix. No heap/buffer. */
 int16_t punch_crush_hold_l,punch_crush_hold_r;
 uint8_t punch_crush_count,punch_crush_wet,punch_radio_wet;
'''+old)
hdr.write_text(s)
s=chip.read_text()
old=' const uint32_t pump_slope=(192u<<12u)/pump_rise;'
assert old in s
s=s.replace(old,old+'''
 /* Compute button parameters ONCE per audio block. Bitcrusher combines true
  * PCM amplitude quantization and zero-order-hold sample-rate reduction;
  * original amplitude-only 8-bit default was inaudible on many FM1 patches.
  * At K1=50: 8-bit at ~2.76 kHz effective sample rate. K1=0:
  * 12-bit at 44.1 kHz; K1=100: 4-bit at ~1.4 kHz. */
 const uint8_t crush_bits=(uint8_t)(12u-(uint32_t)s->punch_param[9]*8u/100u);
 const int32_t crush_quant=(int32_t)(1u<<(16u-crush_bits));
 const uint8_t crush_hold=(uint8_t)(1u+(uint32_t)s->punch_param[9]*30u/100u);
 /* Two cascaded 1-pole highpasses: strong low cut (~180-850Hz nominal
  * cutoff) while leaving all upper frequencies intact (no telephone LP).
  * Coefficient in Q8; runs only when RADIO is held or fading out. */
 const int32_t radio_alpha=7+(int32_t)s->punch_param[14]*21/100;
 const int32_t hiss_level=44+(int32_t)s->punch_param[14]*2;
''')
start=s.index('  if(mask&(1u<<9)){ /* BITCRUSH:')
end=s.index('  if(mask&(1u<<10)){',start)
s=s[:start]+'''  /* True BITCRUSH: not merely chopping inaudible LSBs. The held PCM
   * value updates once every N 44.1kHz frames; thus aliasing/downsampling
   * remains audible even on quieter / smooth synthesized sounds. Avoid
   * resetting the hold counter at each 128-frame DMA block boundary. */
  if(mask&(1u<<9)){if(s->punch_crush_wet<248u)s->punch_crush_wet+=8u;
   else s->punch_crush_wet=255u;}
  else{if(s->punch_crush_wet>=8u)s->punch_crush_wet-=8u;
   else{s->punch_crush_wet=0u;s->punch_crush_count=0u;}}
  if(s->punch_crush_wet){
   if(!s->punch_crush_count){
    /* Division is exact for power-of-two steps. Signed truncation is legal
     * for negative values; avoid left-shifting signed negative samples. */
    int32_t ql=((l>=0?l+crush_quant/2:l-crush_quant/2)/crush_quant)*crush_quant;
    int32_t qr=((r>=0?r+crush_quant/2:r-crush_quant/2)/crush_quant)*crush_quant;
    s->punch_crush_hold_l=(int16_t)punch_clip(ql);
    s->punch_crush_hold_r=(int16_t)punch_clip(qr);
    s->punch_crush_count=crush_hold;
   }
   --s->punch_crush_count;
   const int32_t mix=s->punch_crush_wet;
   l=(l*(256-mix)+(int32_t)s->punch_crush_hold_l*mix)>>8u;
   r=(r*(256-mix)+(int32_t)s->punch_crush_hold_r*mix)>>8u;
  }
'''+s[end:]
start=s.index('  if(mask&(1u<<14)){ /* RADIO:')
end=s.index('  if(mask&(1u<<15)){',start)
s=s[:start]+'''  /* RADIO: a clear LOW-CUT, not a muted telephone band or noise-only
   * generator. Two cascaded highpasses suppress bass without removing highs.
   * Hiss is low-level PCM noise (~-48 dBFS at default), mixed INTO the
   * existing audio. No extra audio-buffer allocation or floating point. */
  if(mask&(1u<<14)){if(s->punch_radio_wet<252u)s->punch_radio_wet+=4u;
   else s->punch_radio_wet=255u;}
  else{if(s->punch_radio_wet>=4u)s->punch_radio_wet-=4u;
   else s->punch_radio_wet=0u;}
  if(s->punch_radio_wet){
   int32_t out_l,out_r;
   int32_t x=l;
   s->punch_radio_lp[0]+=((x-s->punch_radio_lp[0])*radio_alpha)>>8u;
   x-=s->punch_radio_lp[0];
   s->punch_radio_hp[0]+=((x-s->punch_radio_hp[0])*radio_alpha)>>8u;
   out_l=x-s->punch_radio_hp[0];
   x=r;
   s->punch_radio_lp[1]+=((x-s->punch_radio_lp[1])*radio_alpha)>>8u;
   x-=s->punch_radio_lp[1];
   s->punch_radio_hp[1]+=((x-s->punch_radio_hp[1])*radio_alpha)>>8u;
   out_r=x-s->punch_radio_hp[1];
   uint32_t seed=s->punch_fx_seed;
   seed^=seed<<13u;seed^=seed>>17u;seed^=seed<<5u;
   s->punch_fx_seed=seed?seed:0xA718934Fu;
   out_l+=((((int32_t)((seed>>24u)&255u))-128)*hiss_level)>>7u;
   out_r+=((((int32_t)((seed>>16u)&255u))-128)*hiss_level)>>7u;
   const int32_t mix=s->punch_radio_wet;
   l=(l*(256-mix)+out_l*mix)>>8u;
   r=(r*(256-mix)+out_r*mix)>>8u;
  }else{ /* prevent a stale HP state from introducing a click on next press */
   s->punch_radio_lp[0]=l;s->punch_radio_lp[1]=r;
   s->punch_radio_hp[0]=0;s->punch_radio_hp[1]=0;
  }
'''+s[end:]
assert 'BITCRUSH: audible 4..12-bit' not in s
s=s.replace('/* SCRATCH: a stationary stereo vinyl slice','/* CRY (formerly SCRATCH): a stationary stereo vinyl slice')
chip.write_text(s)
p=r/'firmware/fm1-native/track_runtime.h';s=p.read_text()
assert s.count('"SCRATCH"')==2
s=s.replace('"SCRATCH"','"CRY"')
p.write_text(s)
# Simulator native-faithful DSP parity for user-audible changes and current UI.
for fn in ('simulator.html','web/simulator.html'):
 p=r/fn;s=p.read_text()
 assert '"SCRATCH"' not in s # sim single quote names
 assert "'SCRATCH'" in s
 s=s.replace("'SCRATCH'","'CRY'")
 # Processor bitcrush parameters and processing. State lives on processor between blocks.
 old='  const crushBits=4+Math.floor(params[9]*8/100);\n  const crushScale=2**(crushBits-1);'
 assert old in s,fn
 s=s.replace(old,'''  const crushBits=12-Math.floor(params[9]*8/100);
  const crushScale=2**(crushBits-1);
  const crushHold=1+Math.floor(params[9]*30/100);
  this.crushCounter??=0;this.crushL??=0;this.crushR??=0;
  this.crushWet??=0;this.radioWet??=0;''')
 old='   if(active.has(9)){left=Math.round(left*crushScale)/crushScale;right=Math.round(right*crushScale)/crushScale;}'
 assert old in s
 s=s.replace(old,'''   this.crushWet=clamp(this.crushWet+(active.has(9)?8:-8)/256,0,1);
   if(this.crushWet>0){
    if(this.crushCounter<=0){this.crushL=clamp(Math.round(left*crushScale)/crushScale,-1,1);this.crushR=clamp(Math.round(right*crushScale)/crushScale,-1,1);this.crushCounter=crushHold;}
    this.crushCounter--;
    left=left*(1-this.crushWet)+this.crushL*this.crushWet;
    right=right*(1-this.crushWet)+this.crushR*this.crushWet;
   }else this.crushCounter=0;''')
 start=s.index('   if(active.has(14)){')
 end=s.index('   if(active.has(15)){',start)
 s=s[:start]+'''   this.radioWet=clamp(this.radioWet+(active.has(14)?4:-4)/256,0,1);
   if(this.radioWet>0){
    const alpha=(7+Math.floor(params[14]*21/100))/256;
    const hiss=(44+params[14]*2)/32768;
    for(let ch=0;ch<2;ch++){
     const source=ch===0?left:right,st=this.punchRadio[ch];
     st[0]+=(source-st[0])*alpha;
     const hp1=source-st[0];st[1]+=(hp1-st[1])*alpha;
     const lowcut=hp1-st[1]+(Math.random()-.5)*2*hiss;
     const out=source*(1-this.radioWet)+lowcut*this.radioWet;
     if(ch===0)left=out;else right=out;
    }
   }else{this.punchRadio[0][0]=left;this.punchRadio[1][0]=right;this.punchRadio[0][1]=0;this.punchRadio[1][1]=0;}
'''+s[end:]
 p.write_text(s)
# Current README reference only, preserve archival changelog entries.
p=r/'README.md';s=p.read_text();s=s.replace('SCRATCH','CRY');p.write_text(s)
print('Patched native and web DSP; CRY UI and current README')
