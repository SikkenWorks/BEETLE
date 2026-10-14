/* TRACK project format. Pure C, no HAL, no flash, no dynamic memory. */
#include "track_project_store.h"
#include "track_drum.h"
#include <string.h>
static void wr16(uint8_t *p,uint16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void wr32(uint8_t *p,uint32_t n){for(uint8_t i=0;i<4u;i++)p[i]=(uint8_t)(n>>(8u*i));}
static uint16_t rd16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
uint32_t track_store_crc32(const uint8_t *p,uint32_t n){
 uint32_t c=0xFFFFFFFFu;for(uint32_t i=0;i<n;i++){c^=p[i];for(uint8_t k=0;k<8u;k++)c=(c>>1)^((c&1u)?0xEDB88320u:0u);}return ~c;
}
/* V5: six lanes. 12-byte presence mask per pattern, followed by packed
 * 25-bit non-default steps. Older v1..v4 four-lane projects decode with
 * the drum channel remapped CH4 -> CH6. Flash bank stays 12KiB; payloads
 * exceeding that capacity are refused BEFORE any erase (not truncated). */
#define TRACK_PACKED_STEP_BITS 25u
#define TRACK_PACKED_PATTERN_BYTES ((4u*TRACK_STEPS*TRACK_PACKED_STEP_BITS+7u)/8u)
#define TRACK_V5_MASK_BYTES ((TRACK_CHANNELS*TRACK_STEPS+7u)/8u)
static struct track_step empty_step(uint8_t lane){
 struct track_step v={TRACK_NOTE_EMPTY,(uint8_t)(lane==TRACK_DRUM_CHANNEL?3u:(lane&3u)),0u,0u,50u};return v;
}
static uint8_t valid_step(const struct track_step *st,uint8_t lane){
 return st->instrument<=3u && st->effect<TRACK_FX_COUNT && st->value<=100u &&
 (lane==TRACK_DRUM_CHANNEL ?
  ((st->note==TRACK_NOTE_EMPTY||st->note==TRACK_NOTE_STOP||st->note<TRACK_DRUM_COUNT)&&st->chord<=TRACK_DRUM_COUNT) :
  ((st->note==TRACK_NOTE_EMPTY||st->note==TRACK_NOTE_STOP||st->note<=127u)&&st->chord<TRACK_CHORD_COUNT));
}
static void pack_bits(uint8_t *p,uint32_t *bit,uint32_t value,uint8_t count){
 for(uint8_t k=0;k<count;k++){uint32_t at=(*bit)++;p[at>>3]|=(uint8_t)(((value>>k)&1u)<<(at&7u));}
}
static uint32_t unpack_bits(const uint8_t *p,uint32_t *bit,uint8_t count){
 uint32_t v=0u;for(uint8_t k=0;k<count;k++){uint32_t at=(*bit)++;v|=((uint32_t)((p[at>>3]>>(at&7u))&1u))<<k;}return v;
}
#define PUT1(V) do{if(i>=cap)return -2;out[i++]=(uint8_t)(V);}while(0)
#define GET1(V) do{if(i>=n)return -2;(V)=data[i++];}while(0)
int track_store_encode(const struct track_store_project *p,uint8_t *out,uint32_t cap,uint32_t *used){
 if(!p||!out||!used||cap>TRACK_STORE_MAX||cap<40u||p->slot>=TRACK_PROJECTS||p->song.count<1u||p->song.count>TRACK_SONG_ENTRIES)return -1;
 uint32_t i=24u;uint8_t c,r,s;
 for(c=0;c<12u;c++)PUT1(p->title[c]);
 PUT1(p->slot);PUT1(p->harmony.root);PUT1(p->harmony.mode);PUT1(p->harmony.keylock);
 if(i+2u>cap)return -2;
 wr16(out+i,p->bpm);i+=2;
 PUT1(p->song.count);PUT1(p->song.loop);PUT1(p->song.enabled);
 for(c=0;c<p->song.count;c++){PUT1(p->song.chain[c].pattern);PUT1(p->song.chain[c].repeats);}
 for(c=0;c<TRACK_CHANNELS;c++){
  PUT1(p->mixer.ch[c].volume);PUT1((uint8_t)p->mixer.ch[c].pan);
  PUT1(p->mixer.ch[c].mute);PUT1(p->mixer.ch[c].solo);PUT1(p->mixer.ch[c].swing);
 }
 for(r=0;r<TRACK_PATTERNS;r++){
  const struct track_pattern *pat=&p->song.patterns[r];PUT1(pat->length);
  /* One presence bit per step; only populated/non-default cells take 25 bits.
   * Default cells still preserve correct lane instrument and 50% FX value. */
  if(i+TRACK_V5_MASK_BYTES>cap)return -2;
  uint32_t mask_at=i;memset(out+i,0,TRACK_V5_MASK_BYTES);i+=TRACK_V5_MASK_BYTES;
  uint16_t populated=0u;
  for(c=0;c<TRACK_CHANNELS;c++)for(s=0;s<TRACK_STEPS;s++){
   const struct track_step *st=&pat->ch[c][s];
   if(!valid_step(st,c))return -1;
   struct track_step def=empty_step(c);
   if(memcmp(st,&def,sizeof(def))){
    uint32_t pos=(uint32_t)c*TRACK_STEPS+s;
    out[mask_at+(pos>>3)]|=(uint8_t)(1u<<(pos&7u));populated++;
   }
  }
  uint32_t packed=(populated*TRACK_PACKED_STEP_BITS+7u)/8u;
  if(i+packed>cap)return -2;
  memset(out+i,0,packed);uint32_t bit=0u;
  for(c=0;c<TRACK_CHANNELS;c++)for(s=0;s<TRACK_STEPS;s++){
   uint32_t pos=(uint32_t)c*TRACK_STEPS+s;
   if(!(out[mask_at+(pos>>3)]&(1u<<(pos&7u))))continue;
   const struct track_step *st=&pat->ch[c][s];
   pack_bits(out+i,&bit,st->note,8u);pack_bits(out+i,&bit,st->instrument,2u);
   pack_bits(out+i,&bit,st->chord,4u);pack_bits(out+i,&bit,st->effect,4u);
   pack_bits(out+i,&bit,st->value,7u);
  }
  i+=packed;
  const struct track_song_clip *clip=&p->song.clips[r];if(clip->count>TRACK_SONG_FX_EVENTS)return -1;
  PUT1(clip->count);
  for(uint8_t x=0;x<clip->count;x++){
   const struct track_song_event *e=&clip->data[x];
   if(e->step>=TRACK_STEPS||e->fx>=TRACK_PUNCH_COUNT||e->down>2u||
      (e->param>100u&&(e->param!=255u||e->down==2u)))return -1;
   PUT1((uint8_t)(e->step|(e->fx<<4u)|((e->down==0u?0u:1u)<<7u)));
   /* 127 encodes unspecified value for legacy callers (255 in RAM).
    * Bit 7 marks an ALGORITHM change without changing the ON/OFF mask. */
   PUT1(e->down==2u?(uint8_t)(e->param|0x80u):(e->param==255u?127u:e->param));
  }
 }
 for(c=0;c<4u;c++){
  const struct track_store_instrument *v=&p->instruments[c];
  struct track_va_patch def;const struct track_va_patch *vp=&v->va;
  if(!track_va_patch_valid(vp)){
   /* Uninitialized legacy callers never had a VA patch; only an all-zero
    * patch receives defaults. Reject malformed nonzero user patches. */
   static const struct track_va_patch zero={0};
   if(memcmp(vp,&zero,sizeof(zero)))return -1;
   track_va_patch_default(&def);vp=&def;
  }
  if(v->type>=TRACK_CHIP_TYPES||v->duty>100u||v->volume>100u)return -1;
  PUT1(v->type);PUT1(v->attack);PUT1(v->decay);PUT1(v->sustain);PUT1(v->release);
  PUT1(v->duty);PUT1(v->volume);if(i+2u>cap)return -2;wr16(out+i,v->cutoff);i+=2;
  PUT1(vp->wave_a);PUT1(vp->wave_b);PUT1((uint8_t)vp->coarse);PUT1((uint8_t)vp->fine);
  PUT1(vp->detune);PUT1(vp->mix);PUT1(vp->pulse_width);PUT1(vp->morph);
  PUT1((uint8_t)vp->coarse_b);PUT1((uint8_t)vp->fine_b);PUT1(vp->pulse_width_b);
  PUT1(vp->drive);PUT1(vp->sub);PUT1(vp->noise);PUT1(vp->fm_depth);
 }
 for(c=0u;c<TRACK_DRUM_COUNT;c++){
  const struct track_drum_timbre *d=&p->drum[c];
  PUT1(d->attack);PUT1(d->decay);PUT1(d->sustain);PUT1(d->release);
  PUT1(d->eq_low);PUT1(d->eq_mid);PUT1(d->eq_high);PUT1(d->gain);PUT1((uint8_t)d->pitch);
 }
 for(c=0u;c<4u;c++)PUT1(p->master.eq[c]);
 for(c=0u;c<4u;c++)PUT1(p->master.eq_freq[c]);
 for(c=0u;c<4u;c++)PUT1(p->master.eq_width[c]);
 PUT1(p->master.comp_threshold);PUT1(p->master.comp_ratio);PUT1(p->master.comp_gain);
 PUT1(p->master.reverb_wet);PUT1(p->master.reverb_feedback);PUT1(p->master.reverb_tone);
 for(c=0u;c<3u;c++)PUT1(p->master.enabled[c]);
 for(c=0u;c<3u;c++)PUT1(p->master.order[c]);
 for(c=0u;c<5u;c++){if(p->punch_param[c]>100u)return -1;PUT1(p->punch_param[c]);}
 if(p->master.comp_attack>100u||p->master.reverb_spread>100u)return -1;
 PUT1(p->master.comp_attack);PUT1(p->master.reverb_spread);
 wr32(out,TRACK_STORE_MAGIC);wr16(out+4,TRACK_STORE_VERSION);
 wr16(out+6,(uint16_t)p->slot);wr32(out+8,i);wr32(out+12,track_store_crc32(out+24u,i-24u));
 wr32(out+16,track_store_crc32(out,16u));wr32(out+20,0u);
 *used=i;return 0;
}
int track_store_decode(struct track_store_project *p,const uint8_t *data,uint32_t n){
 if(!p||!data||n<42u||n>TRACK_STORE_MAX)return -1;
 if(rd32(data)!=TRACK_STORE_MAGIC||(rd16(data+4u)!=1u&&rd16(data+4u)!=2u&&rd16(data+4u)!=3u&&rd16(data+4u)!=4u&&rd16(data+4u)!=5u&&rd16(data+4u)!=6u&&rd16(data+4u)!=7u&&rd16(data+4u)!=8u&&rd16(data+4u)!=9u&&rd16(data+4u)!=TRACK_STORE_VERSION)||rd16(data+6u)>=TRACK_PROJECTS||rd32(data+8u)!=n||
    rd32(data+12u)!=track_store_crc32(data+24u,n-24u)||rd32(data+16u)!=track_store_crc32(data,16u))return -3;
 uint32_t i=24u;uint8_t c,r,s;
 memset(p,0,sizeof(*p));
 for(c=0;c<12u;c++){GET1(p->title[c]);}
 p->title[12]=0;
 GET1(p->slot);GET1(p->harmony.root);GET1(p->harmony.mode);GET1(p->harmony.keylock);
 if(i+2u>n)return -2;
 p->bpm=rd16(data+i);i+=2;
 GET1(p->song.count);GET1(p->song.loop);GET1(p->song.enabled);
 if(p->slot>=TRACK_PROJECTS||p->harmony.root>=12u||p->harmony.mode>=9u||p->harmony.keylock>2u||p->bpm<30u||p->bpm>300u||p->song.count<1u||p->song.count>TRACK_SONG_ENTRIES)return -3;
 for(c=0;c<p->song.count;c++){
  GET1(p->song.chain[c].pattern);GET1(p->song.chain[c].repeats);
  if(p->song.chain[c].pattern>=TRACK_PATTERNS||p->song.chain[c].repeats<1u||p->song.chain[c].repeats>16u)return -3;
 }
 for(c=0;c<TRACK_CHANNELS;c++){p->mixer.ch[c].volume=80u;p->mixer.ch[c].pan=0u;}
 uint8_t v5=(uint8_t)(rd16(data+4u)>=5u);
 for(c=0;c<(v5?TRACK_CHANNELS:4u);c++){
  uint8_t lane=(uint8_t)((!v5&&c==3u)?TRACK_DRUM_CHANNEL:c);
  struct track_mixer_channel *m=&p->mixer.ch[lane];
  GET1(m->volume);GET1(s);m->pan=(int8_t)s;
  GET1(m->mute);GET1(m->solo);
  if(rd16(data+4u)>=3u){GET1(m->swing);if(rd16(data+4u)<7u)m->swing=m->swing?50u:0u;}else m->swing=0u;
  if(m->volume>100u||m->pan< -100||m->pan>100||
     m->mute>1u||m->solo>1u||m->swing>100u)return -3;
  /* V0.45 initialized the drum fader at 100 to compensate for quiet PCM.
   * Migrate that specific old factory default to 80; V0.46 stores the
   * user's own slider choice verbatim (including a deliberate 100). */
  if(rd16(data+4u)==8u&&lane==TRACK_DRUM_CHANNEL&&m->volume==100u)m->volume=80u;
 }
 for(r=0;r<TRACK_PATTERNS;r++){
  struct track_pattern *pat=&p->song.patterns[r];GET1(pat->length);
  if(pat->length<1u||pat->length>TRACK_STEPS)return -3;
  for(c=0;c<TRACK_CHANNELS;c++)for(s=0;s<TRACK_STEPS;s++)pat->ch[c][s]=empty_step(c);
  uint32_t bit=0u,packed=0u;uint8_t present[TRACK_V5_MASK_BYTES];
  if(v5){
   if(i+TRACK_V5_MASK_BYTES>n)return -2;
   memcpy(present,data+i,TRACK_V5_MASK_BYTES);i+=TRACK_V5_MASK_BYTES;
   uint32_t count=0u;
   for(uint8_t x=0u;x<TRACK_V5_MASK_BYTES;x++)for(uint8_t b=0u;b<8u;b++)count+=(present[x]>>b)&1u;
   packed=(count*TRACK_PACKED_STEP_BITS+7u)/8u;
   if(i+packed>n)return -2;
  }else if(rd16(data+4u)>=4u){
   packed=TRACK_PACKED_PATTERN_BYTES;if(i+packed>n)return -2;
  }
  for(c=0;c<(v5?TRACK_CHANNELS:4u);c++)for(s=0;s<TRACK_STEPS;s++){
   uint8_t lane=(uint8_t)((!v5&&c==3u)?TRACK_DRUM_CHANNEL:c);
   struct track_step *st=&pat->ch[lane][s];
   if(v5 && !(present[((uint32_t)c*TRACK_STEPS+s)>>3u]&(1u<<(((uint32_t)c*TRACK_STEPS+s)&7u))))continue;
   if(rd16(data+4u)>=4u){
    st->note=(uint8_t)unpack_bits(data+i,&bit,8u);
    st->instrument=(uint8_t)unpack_bits(data+i,&bit,2u);
    st->chord=(uint8_t)unpack_bits(data+i,&bit,4u);
    st->effect=(uint8_t)unpack_bits(data+i,&bit,4u);
    st->value=(uint8_t)unpack_bits(data+i,&bit,7u);
   }else{
    GET1(st->note);GET1(st->instrument);GET1(st->chord);GET1(st->effect);GET1(st->value);
   }
   if(!valid_step(st,lane))return -3;
  }
  if(rd16(data+4u)>=4u)i+=packed;
  struct track_song_clip *clip=&p->song.clips[r];GET1(clip->count);
  if(clip->count>TRACK_SONG_FX_EVENTS)return -3;
  for(uint8_t x=0;x<clip->count;x++){
   if(rd16(data+4u)>=4u){uint8_t ev;GET1(ev);
    clip->data[x].step=ev&15u;clip->data[x].fx=(ev>>4u)&7u;clip->data[x].down=ev>>7u;
    clip->data[x].param=255u;
    if(rd16(data+4u)>=9u){uint8_t aux;GET1(aux);
     if(aux&0x80u){clip->data[x].down=2u;clip->data[x].param=(uint8_t)(aux&0x7Fu);}
     else clip->data[x].param=(aux==127u?255u:aux);
    }
   }else{
    GET1(clip->data[x].step);GET1(clip->data[x].fx);GET1(clip->data[x].down);
    clip->data[x].param=255u;
   }
   if(clip->data[x].step>=TRACK_STEPS||clip->data[x].fx>=TRACK_PUNCH_COUNT||clip->data[x].down>2u||
      (clip->data[x].param>100u&&clip->data[x].param!=255u))return -3;
  }
 }
 if(rd16(data+4u)>=2u){
  for(c=0;c<4u;c++){
   struct track_store_instrument *v=&p->instruments[c];
   GET1(v->type);GET1(v->attack);GET1(v->decay);GET1(v->sustain);GET1(v->release);
   GET1(v->duty);GET1(v->volume);if(i+2u>n)return -2;v->cutoff=rd16(data+i);i+=2;
   GET1(v->va.wave_a);GET1(v->va.wave_b);GET1(s);v->va.coarse=(int8_t)s;
   GET1(s);v->va.fine=(int8_t)s;GET1(v->va.detune);GET1(v->va.mix);
   GET1(v->va.pulse_width);GET1(v->va.morph);
   if(rd16(data+4u)>=4u){GET1(s);v->va.coarse_b=(int8_t)s;
    GET1(s);v->va.fine_b=(int8_t)s;GET1(v->va.pulse_width_b);}
   else{v->va.coarse_b=v->va.coarse;v->va.fine_b=v->va.fine;
    v->va.pulse_width_b=v->va.pulse_width;}
   if(rd16(data+4u)>=6u){GET1(v->va.drive);GET1(v->va.sub);GET1(v->va.noise);GET1(v->va.fm_depth);}
   else {v->va.drive=0u;v->va.sub=0u;v->va.noise=0u;v->va.fm_depth=0u;}
   if(!track_va_patch_valid(&v->va)||v->type>=TRACK_CHIP_TYPES||v->duty>100u||v->volume>100u)return -3;
  }
 }else{ /* Legacy TRACK v1: preserve all patterns, supply default patch knobs. */
  for(c=0;c<4u;c++){
   struct track_store_instrument *v=&p->instruments[c];
   v->type=c;v->attack=1u;v->decay=14u;v->sustain=100u;v->release=16u;
   v->duty=50u;v->cutoff=7000u;v->volume=65u;track_va_patch_default(&v->va);
  }
 }
 if(rd16(data+4u)>=7u){
  for(c=0u;c<TRACK_DRUM_COUNT;c++){
   struct track_drum_timbre *d=&p->drum[c];
   GET1(d->attack);GET1(d->decay);GET1(d->sustain);GET1(d->release);
   GET1(d->eq_low);GET1(d->eq_mid);GET1(d->eq_high);GET1(d->gain);GET1(s);d->pitch=(int8_t)s;
   if(d->attack>100u||d->decay>100u||d->sustain>100u||d->release>100u||
     d->eq_low>100u||d->eq_mid>100u||d->eq_high>100u||d->gain>100u||d->pitch< -36||d->pitch>36)return -3;
  }
  for(c=0u;c<4u;c++){GET1(p->master.eq[c]);if(p->master.eq[c]>100u)return -3;}
  for(c=0u;c<4u;c++){GET1(p->master.eq_freq[c]);if(p->master.eq_freq[c]>100u)return -3;}
  for(c=0u;c<4u;c++){GET1(p->master.eq_width[c]);if(p->master.eq_width[c]>100u)return -3;}
  GET1(p->master.comp_threshold);GET1(p->master.comp_ratio);GET1(p->master.comp_gain);
  GET1(p->master.reverb_wet);GET1(p->master.reverb_feedback);GET1(p->master.reverb_tone);
  for(c=0u;c<3u;c++){GET1(p->master.enabled[c]);if(p->master.enabled[c]>1u)return -3;}
  uint8_t seen=0u;
  for(c=0u;c<3u;c++){GET1(p->master.order[c]);if(p->master.order[c]>2u||(seen&(1u<<p->master.order[c])))return -3;seen|=(uint8_t)(1u<<p->master.order[c]);}
  if(p->master.comp_threshold>100u||p->master.comp_ratio>100u||p->master.comp_gain>100u||p->master.reverb_wet>100u||p->master.reverb_feedback>100u||p->master.reverb_tone>100u)return -3;
 }else{
  for(c=0u;c<TRACK_DRUM_COUNT;c++)p->drum[c]=(struct track_drum_timbre){0u,50u,0u,50u,50u,50u,50u,100u,0};
  for(c=0u;c<4u;c++){p->master.eq[c]=50u;p->master.eq_freq[c]=50u;p->master.eq_width[c]=50u;}
  p->master.comp_threshold=75u;p->master.comp_ratio=40u;p->master.comp_gain=50u;
  p->master.reverb_wet=15u;p->master.reverb_feedback=32u;p->master.reverb_tone=50u;
  for(c=0u;c<3u;c++){p->master.enabled[c]=1u;p->master.order[c]=c;}
 }
 if(rd16(data+4u)>=8u){
  for(c=0u;c<5u;c++){GET1(p->punch_param[c]);if(p->punch_param[c]>100u)return -3;}
 }else{
  for(c=0u;c<5u;c++)p->punch_param[c]=50u;
  p->punch_param[2]=0u;
 }
 if(rd16(data+4u)>=10u){
  GET1(p->master.comp_attack);GET1(p->master.reverb_spread);
  if(p->master.comp_attack>100u||p->master.reverb_spread>100u)return -3;
 }else{p->master.comp_attack=28u;p->master.reverb_spread=50u;}
 if(i!=n)return -3;
 return 0;
}
int track_store_can_claim(const uint8_t *sector,uint32_t size){
 if(!sector||size<32u)return 0;
 for(uint32_t i=0;i<size;i++)if(sector[i]!=0xFFu){
  /* Existing sector content: do not claim or erase foreign data.
   * Valid TRK1 is not sufficient to authorize writing on its own.
   * Future backend requires explicit user opt-in and proven journal state. */
  return 0;
 }
 return 1;
}
