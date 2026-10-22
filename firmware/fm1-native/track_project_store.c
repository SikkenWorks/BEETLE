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
#define TRACK_PACKED_STEP_BITS 25u /* V4-V10 */
#define TRACK_V11_PACKED_STEP_BITS 33u /* adds 8 dynamics bits */
#define TRACK_V12_PACKED_STEP_BITS 40u
#define TRACK_V20_PACKED_STEP_BITS 41u /* additional 7 groove bits */
#define TRACK_PACKED_PATTERN_BYTES ((4u*TRACK_STEPS*TRACK_PACKED_STEP_BITS+7u)/8u)
#define TRACK_V5_MASK_BYTES ((TRACK_CHANNELS*TRACK_STEPS+7u)/8u)
static struct track_step empty_step(uint8_t lane){
 struct track_step v={TRACK_NOTE_EMPTY,(uint8_t)(lane==TRACK_DRUM_CHANNEL?3u:lane),0u,0u,50u,0u,0u};return v;
}
static uint8_t valid_step(const struct track_step *st,uint8_t lane){
 return st->instrument<=(lane==TRACK_DRUM_CHANNEL?3u:7u) && st->effect<TRACK_FX_COUNT && st->value<=100u &&
 (lane==TRACK_DRUM_CHANNEL ?
  ((st->note==TRACK_NOTE_EMPTY||st->note==TRACK_NOTE_STOP||st->note<TRACK_DRUM_NOTE_COUNT)&&st->chord<=15u) :
  ((st->note==TRACK_NOTE_EMPTY||st->note==TRACK_NOTE_STOP||st->note<=127u)&&st->chord<TRACK_CHORD_COUNT));
}
static void pack_bits(uint8_t *p,uint32_t *bit,uint32_t value,uint8_t count){
 for(uint8_t k=0;k<count;k++){uint32_t at=(*bit)++;p[at>>3]|=(uint8_t)(((value>>k)&1u)<<(at&7u));}
}
static uint32_t unpack_bits(const uint8_t *p,uint32_t *bit,uint8_t count){
 uint32_t v=0u;for(uint8_t k=0;k<count;k++){uint32_t at=(*bit)++;v|=((uint32_t)((p[at>>3]>>(at&7u))&1u))<<k;}return v;
}
/* V0.74 fail-closed preflight: the prior encoder checked only capacity and
 * selected fields, so it could commit a CRC-correct project that the decoder
 * would subsequently reject (e.g. BPM 0, an invalid song chain, or an invalid
 * master FX order). Validate the actual current V17 on-flash contract BEFORE asking
 * the journal backend to erase a bank. No schema or flash layout changes. */
static uint8_t track_store_preflight(const struct track_store_project *p){
 if(p->slot>=TRACK_PROJECTS || p->bpm<30u || p->bpm>300u ||
    p->harmony.root>=12u || p->harmony.mode>=9u || p->harmony.keylock>2u ||
    p->song.count<1u || p->song.count>TRACK_SONG_ENTRIES ||
    p->song.loop>1u || p->song.enabled>1u || p->mixer.quantize_mutes>1u)return 0u;
 for(uint8_t k=0u;k<p->song.count;k++)
  if(p->song.chain[k].pattern>=TRACK_PATTERNS ||
     p->song.chain[k].repeats<1u || p->song.chain[k].repeats>16u)return 0u;
 for(uint8_t k=0u;k<p->song.count;k++)for(uint8_t c=0u;c<TRACK_CHANNELS;c++)
  if(!track_song_valid_lane(p->song.chain[k].lane[c]))return 0u;
 for(uint8_t k=0u;k<TRACK_CHANNELS;k++){
  const struct track_mixer_channel *m=&p->mixer.ch[k];
  if(m->volume>120u || m->pan< -100 || m->pan>100 || m->mute>1u ||
     m->solo>1u || m->swing>100u)return 0u;
 }
 for(uint8_t k=0u;k<TRACK_PATTERNS;k++){
  const struct track_pattern *pat=&p->song.patterns[k];
  if(pat->length<1u || pat->length>TRACK_STEPS || p->song.extras.count>TRACK_EXTRA_FX_CAP)return 0u;
  for(uint8_t x=0u;x<p->song.extras.count;x++){
   const struct track_extra_fx *f=&p->song.extras.data[x];
   if(f->pattern!=k)continue;
   uint8_t key=f->id,lane=(uint8_t)((key&127u)/TRACK_STEPS),row=(uint8_t)((key&127u)%TRACK_STEPS);
   if(lane>=TRACK_CHANNELS||((key&128u)!=0u&&lane!=TRACK_DRUM_CHANNEL)||
      track_extra_effect(f)==0u||track_extra_effect(f)>=TRACK_FX_COUNT||f->value>100u)return 0u;
   if(track_extra_count(&p->song,k,key)>2u)return 0u;
  }
  for(uint8_t s=0u;s<TRACK_STEPS;s++)
   if(pat->note2_fx[s].effect>=TRACK_FX_COUNT||pat->note2_fx[s].value>100u)return 0u;
 }
 for(uint8_t k=0u;k<TRACK_DRUM_COUNT;k++){
  const struct track_drum_timbre *d=&p->drum[k];
  if(d->attack>100u || d->decay>100u || d->sustain>100u || d->release>100u ||
     d->eq_low>100u || d->eq_mid>100u || d->eq_high>100u ||
     d->gain>100u || d->pitch< -36 || d->pitch>36)return 0u;
 }
 const struct track_master_settings *m=&p->master;
 for(uint8_t k=0u;k<4u;k++)
  if(m->eq[k]>100u || m->eq_freq[k]>100u || m->eq_width[k]>100u)return 0u;
 if(m->comp_threshold>100u || m->comp_ratio>100u || m->comp_gain>100u ||
    m->comp_attack>100u || m->reverb_wet>100u || m->reverb_feedback>100u ||
    m->reverb_tone>100u || m->reverb_spread>100u)return 0u;
 uint8_t order_seen=0u;
 for(uint8_t k=0u;k<3u;k++){
  if(m->enabled[k]>1u || m->order[k]>2u || (order_seen&(1u<<m->order[k])))return 0u;
  order_seen|=(uint8_t)(1u<<m->order[k]);
 }
 return 1u;
}
#define PUT1(V) do{if(i>=cap)return -2;out[i++]=(uint8_t)(V);}while(0)
#define GET1(V) do{if(i>=n)return -2;(V)=data[i++];}while(0)
int track_store_encode(const struct track_store_project *p,uint8_t *out,uint32_t cap,uint32_t *used){
 if(!p||!out||!used||cap>TRACK_STORE_MAX||cap<40u||!track_store_preflight(p))return -1;
 uint32_t i=24u;uint8_t c,r,s;
 for(c=0;c<12u;c++)PUT1(p->title[c]);
 PUT1(p->slot);PUT1(p->harmony.root);PUT1(p->harmony.mode);PUT1(p->harmony.keylock);
 if(i+2u>cap)return -2;
 wr16(out+i,p->bpm);i+=2;
 PUT1(p->song.count);PUT1(p->song.loop);PUT1(p->song.enabled);
 for(c=0;c<p->song.count;c++){PUT1(p->song.chain[c].pattern);PUT1(p->song.chain[c].repeats);
  for(uint8_t lane=0u;lane<TRACK_CHANNELS;lane++)PUT1(p->song.chain[c].lane[lane]);}
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
   /* Compare semantic fields, not packed-bitfield padding. UBSan and
    * different compilers can give the unused bit a different value. */
   if(st->note!=def.note || st->instrument!=def.instrument ||
      st->chord!=def.chord || st->effect!=def.effect ||
      st->value!=def.value || st->dynamics!=def.dynamics ||
      st->groove!=def.groove){
    uint32_t pos=(uint32_t)c*TRACK_STEPS+s;
    out[mask_at+(pos>>3)]|=(uint8_t)(1u<<(pos&7u));populated++;
   }
  }
  uint32_t packed=(populated*TRACK_V20_PACKED_STEP_BITS+7u)/8u;
  if(i+packed>cap)return -2;
  memset(out+i,0,packed);uint32_t bit=0u;
  for(c=0;c<TRACK_CHANNELS;c++)for(s=0;s<TRACK_STEPS;s++){
   uint32_t pos=(uint32_t)c*TRACK_STEPS+s;
   if(!(out[mask_at+(pos>>3)]&(1u<<(pos&7u))))continue;
   const struct track_step *st=&pat->ch[c][s];
   pack_bits(out+i,&bit,st->note,8u);pack_bits(out+i,&bit,st->instrument,3u);
   pack_bits(out+i,&bit,st->chord,4u);pack_bits(out+i,&bit,st->effect,4u);
   pack_bits(out+i,&bit,st->value,7u);
   pack_bits(out+i,&bit,st->dynamics,8u);pack_bits(out+i,&bit,st->groove,7u);
  }
  i+=packed;
  /* V18: compact independent CH6 NOTE2 FX: two-byte bitmask + FX/value pairs.
   * A blank pair (0/0 or 0/50) consumes no storage. Older schemas are decoded below. */
  uint16_t fx2_mask=0u;
  for(s=0;s<TRACK_STEPS;s++){
   const struct track_note2_fx *f=&pat->note2_fx[s];
   if(f->effect || (f->value && f->value!=50u))fx2_mask|=(uint16_t)(1u<<s);
  }
  if(i+2u>cap)return -2;
  wr16(out+i,fx2_mask);i+=2u;
  for(s=0;s<TRACK_STEPS;s++)if(fx2_mask&(1u<<s)){
   PUT1(pat->note2_fx[s].effect);PUT1(pat->note2_fx[s].value);
  }
  /* V21: per-pattern sparse slice of the global FX pool. */
  uint8_t extra_count=0u;
  for(uint8_t x=0u;x<p->song.extras.count;x++)
   if(p->song.extras.data[x].pattern==r)extra_count++;
  PUT1(extra_count);
  for(uint8_t x=0u;x<p->song.extras.count;x++){
   const struct track_extra_fx *f=&p->song.extras.data[x];if(f->pattern!=r)continue;
   PUT1(f->id);PUT1(f->effect);PUT1(f->value);
  }
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
 for(c=0;c<TRACK_CHIP_SLOTS;c++){
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
  PUT1(v->duty);PUT1(v->volume);if(i+2u>cap)return -2;
  wr16(out+i,v->cutoff);i+=2;
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
 if(p->mixer.quantize_mutes>1u)return -1;PUT1(p->mixer.quantize_mutes);
 if(p->snapshots.valid & (uint8_t)~0x0Fu)return -1;
 PUT1(p->snapshots.valid);
 for(c=0u;c<BEETLE_SNAPSHOT_SLOTS;c++){
  if(p->snapshots.masks[c] & (uint8_t)~BEETLE_SNAPSHOT_MASK)return -1;
  PUT1(p->snapshots.masks[c]);
 }
 for(c=0u;c<BEETLE_GROUP_SLOTS;c++){
  if(p->groups.masks[c] & (uint8_t)~BEETLE_GROUP_MASK)return -1;
  PUT1(p->groups.masks[c]);
 }

 /* V16: eight project-local named instrument slots, encoded only when valid.
  * Adding a library never claims another flash sector or changes the A/B layout. */
 PUT1(p->library.valid);
 for(c=0u;c<BEETLE_LIBRARY_SLOTS;c++){
  if(!(p->library.valid&(1u<<c)))continue;
  const struct track_store_instrument *v=&p->library.patch[c];
  if(v->type>=TRACK_CHIP_TYPES||v->duty>100u||v->volume>100u||!track_va_patch_valid(&v->va))return -1;
  for(uint8_t j=0u;j<BEETLE_LIBRARY_NAME;j++){
   char ch=p->library.name[c][j];
   if(!((ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9')||ch==' '||ch=='-'||ch=='_'))return -1;
   PUT1(ch);
  }
  PUT1(v->type);PUT1(v->attack);PUT1(v->decay);PUT1(v->sustain);PUT1(v->release);
  PUT1(v->duty);PUT1(v->volume);
  if(i+2u>cap)return -2;
  wr16(out+i,v->cutoff);i+=2;
  const struct track_va_patch *vp=&v->va;
  PUT1(vp->wave_a);PUT1(vp->wave_b);PUT1((uint8_t)vp->coarse);PUT1((uint8_t)vp->fine);
  PUT1(vp->detune);PUT1(vp->mix);PUT1(vp->pulse_width);PUT1(vp->morph);
  PUT1((uint8_t)vp->coarse_b);PUT1((uint8_t)vp->fine_b);PUT1(vp->pulse_width_b);
  PUT1(vp->drive);PUT1(vp->sub);PUT1(vp->noise);PUT1(vp->fm_depth);
 }
 /* V17: a type can have its own ADSR, oscillator, cutoff and tuning.
  * Valid mask avoids wasting project space for unused waveform types. */
 for(c=0u;c<TRACK_CHIP_SLOTS;c++){
  if(p->type_valid[c] & (uint16_t)~((1u<<TRACK_CHIP_TYPES)-1u))return -1;
  if(i+2u>cap)return -2;
  wr16(out+i,p->type_valid[c]);i+=2;
  for(uint8_t t=0;t<TRACK_CHIP_TYPES;t++)if(p->type_valid[c]&(1u<<t)){
   const struct track_store_instrument *v=&p->type_presets[c][t];
   if(v->type!=t||v->duty>100u||v->volume>100u||
      v->attack>100u||v->decay>100u||v->sustain>100u||v->release>100u||
      !track_va_patch_valid(&v->va))return -1;
   PUT1(v->type);PUT1(v->attack);PUT1(v->decay);PUT1(v->sustain);PUT1(v->release);
   PUT1(v->duty);PUT1(v->volume);
   if(i+2u>cap)return -2;
  wr16(out+i,v->cutoff);i+=2;
   const struct track_va_patch *vp=&v->va;
   PUT1(vp->wave_a);PUT1(vp->wave_b);PUT1((uint8_t)vp->coarse);PUT1((uint8_t)vp->fine);
   PUT1(vp->detune);PUT1(vp->mix);PUT1(vp->pulse_width);PUT1(vp->morph);
   PUT1((uint8_t)vp->coarse_b);PUT1((uint8_t)vp->fine_b);PUT1(vp->pulse_width_b);
   PUT1(vp->drive);PUT1(vp->sub);PUT1(vp->noise);PUT1(vp->fm_depth);
  }
 }
 /* V23: compact user one-shots, no reserved flash sectors. The maximum
  * encoded size remains constrained by the existing A/B project journal.
  * Layout: per slot [length16,name8], followed by concatenated µ-law data;
  * the final 6 bytes carry the base offset and B1OS signature for the Manager.
  * Reject size overrun BEFORE returning a valid CRC or touching flash. */
 {uint32_t total=beetle_oneshot_used(&p->oneshots);
  if(total>BEETLE_ONESHOT_MAX_BYTES)return -2;
  if(total){
  if(i+BEETLE_ONESHOT_NOTES*10u+total+6u>cap)return -2;
  uint16_t start=(uint16_t)i;
  for(uint8_t x=0;x<BEETLE_ONESHOT_NOTES;x++){
   wr16(out+i,p->oneshots.length[x]);i+=2u;
   for(uint8_t y=0;y<8u;y++){
    uint8_t ch=(uint8_t)p->oneshots.name[x][y];
    if(ch!=0u && !(ch>='A'&&ch<='Z') && !(ch>='0'&&ch<='9') && ch!=' ' && ch!='-' && ch!='_')return -1;
    out[i++]=ch;
   }
  }
  if(total && (!p->oneshots.first || (total>BEETLE_ONESHOT_FIRST_BYTES&&!p->oneshots.second)))return -1;
  for(uint32_t x=0u;x<total;x++)out[i++]=beetle_oneshot_get(&p->oneshots,x);
  wr16(out+i,start);i+=2u;
  out[i++]='B';out[i++]='1';out[i++]='O';out[i++]='S';
  }
 }
 wr32(out,TRACK_STORE_MAGIC);wr16(out+4,TRACK_STORE_VERSION);
 wr16(out+6,(uint16_t)p->slot);wr32(out+8,i);wr32(out+12,track_store_crc32(out+24u,i-24u));
 wr32(out+16,track_store_crc32(out,16u));wr32(out+20,0u);
 *used=i;return 0;
}
int track_store_decode(struct track_store_project *p,const uint8_t *data,uint32_t n){
 if(!p||!data||n<42u||n>TRACK_STORE_MAX)return -1;
 if(rd32(data)!=TRACK_STORE_MAGIC||(rd16(data+4u)!=1u&&rd16(data+4u)!=2u&&rd16(data+4u)!=3u&&rd16(data+4u)!=4u&&rd16(data+4u)!=5u&&rd16(data+4u)!=6u&&rd16(data+4u)!=7u&&rd16(data+4u)!=8u&&rd16(data+4u)!=9u&&rd16(data+4u)!=10u&&rd16(data+4u)!=11u&&rd16(data+4u)!=12u&&rd16(data+4u)!=13u&&rd16(data+4u)!=14u&&rd16(data+4u)!=15u&&rd16(data+4u)!=16u&&rd16(data+4u)!=17u&&rd16(data+4u)!=18u&&rd16(data+4u)!=19u&&rd16(data+4u)!=20u&&rd16(data+4u)!=21u&&rd16(data+4u)!=22u&&rd16(data+4u)!=23u&&rd16(data+4u)!=TRACK_STORE_VERSION)||rd16(data+6u)>=TRACK_PROJECTS||rd32(data+8u)!=n||
    rd32(data+12u)!=track_store_crc32(data+24u,n-24u)||rd32(data+16u)!=track_store_crc32(data,16u))return -3;
 /* Fail closed on a structurally invalid V23 suffix before touching the
  * live staging bank or the RAM samples still referenced by the drum ISR. */
 if(rd16(data+4u)==23u || (rd16(data+4u)==24u && n>=6u &&
     data[n-4u]=='B' && data[n-3u]=='1' && data[n-2u]=='O' && data[n-1u]=='S')){
  if(n<BEETLE_ONESHOT_NOTES*10u+6u)return -3;
  uint32_t start=rd16(data+n-6u),pos=start,total=0u;
  if(start<42u||start+BEETLE_ONESHOT_NOTES*10u+6u>n||
     data[n-4u]!='B'||data[n-3u]!='1'||data[n-2u]!='O'||data[n-1u]!='S')return -3;
  for(uint8_t x=0u;x<BEETLE_ONESHOT_NOTES;x++){
   uint16_t len=rd16(data+pos);pos+=2u;total+=len;
   if(total>BEETLE_ONESHOT_MAX_BYTES)return -3;
   for(uint8_t y=0u;y<8u;y++){
    uint8_t ch=data[pos++];
    if(ch && !(ch>='A'&&ch<='Z') && !(ch>='0'&&ch<='9') && ch!=' '&&ch!='-'&&ch!='_')return -3;
   }
  }
  if(pos+total+6u!=n || (total && !p->oneshots.first) ||
     (total>BEETLE_ONESHOT_FIRST_BYTES && !p->oneshots.second))return -3;
 }
 uint32_t i=24u;uint8_t c,r,s;
 uint8_t *samples_first=p->oneshots.first,*samples_second=p->oneshots.second;
 memset(p,0,sizeof(*p));
 p->oneshots.first=samples_first;p->oneshots.second=samples_second;
 for(c=0;c<12u;c++){GET1(p->title[c]);}
 p->title[12]=0;
 GET1(p->slot);GET1(p->harmony.root);GET1(p->harmony.mode);GET1(p->harmony.keylock);
 if(i+2u>n)return -2;
 p->bpm=rd16(data+i);i+=2;
 GET1(p->song.count);GET1(p->song.loop);GET1(p->song.enabled);
 if(p->slot>=TRACK_PROJECTS||p->harmony.root>=12u||p->harmony.mode>=9u||p->harmony.keylock>2u||p->bpm<30u||p->bpm>300u||p->song.count<1u||p->song.count>TRACK_SONG_ENTRIES||p->song.loop>1u||p->song.enabled>1u)return -3;
 for(c=0;c<p->song.count;c++){
  GET1(p->song.chain[c].pattern);GET1(p->song.chain[c].repeats);
  if(p->song.chain[c].pattern>=TRACK_PATTERNS||p->song.chain[c].repeats<1u||p->song.chain[c].repeats>16u)return -3;
  for(uint8_t lane=0u;lane<TRACK_CHANNELS;lane++){
   if(rd16(data+4u)>=19u){GET1(p->song.chain[c].lane[lane]);}
   else p->song.chain[c].lane[lane]=p->song.chain[c].pattern;
   if(!track_song_valid_lane(p->song.chain[c].lane[lane]) ||
      (rd16(data+4u)<24u&&p->song.chain[c].lane[lane]>=TRACK_SONG_AUTO_BASE&&p->song.chain[c].lane[lane]!=TRACK_SONG_SILENT))return -3;
  }
 }
 for(c=0;c<TRACK_CHANNELS;c++){p->mixer.ch[c].volume=c==TRACK_DRUM_CHANNEL?80u:50u;p->mixer.ch[c].pan=0u;}
 uint8_t v5=(uint8_t)(rd16(data+4u)>=5u);
 for(c=0;c<(v5?TRACK_CHANNELS:4u);c++){
  uint8_t lane=(uint8_t)((!v5&&c==3u)?TRACK_DRUM_CHANNEL:c);
  struct track_mixer_channel *m=&p->mixer.ch[lane];
  GET1(m->volume);GET1(s);m->pan=(int8_t)s;
  GET1(m->mute);GET1(m->solo);
  if(rd16(data+4u)>=3u){GET1(m->swing);if(rd16(data+4u)<7u)m->swing=m->swing?50u:0u;}else m->swing=0u;
  if(m->volume>120u||m->pan< -100||m->pan>100||
     m->mute>1u||m->solo>1u||m->swing>100u)return -3;
  /* V0.45 initialized the drum fader at 100 to compensate for quiet PCM.
   * Migrate that specific old factory default to 80; V0.46 stores the
   * user's own slider choice verbatim (including a deliberate 100). */
  if(rd16(data+4u)==8u&&lane==TRACK_DRUM_CHANNEL&&m->volume==100u)m->volume=80u;
 }
 for(r=0;r<TRACK_PATTERNS;r++){
  struct track_pattern *pat=&p->song.patterns[r];GET1(pat->length);
  if(pat->length<1u||pat->length>TRACK_STEPS)return -3;
  for(c=0;c<TRACK_CHANNELS;c++)for(s=0;s<TRACK_STEPS;s++){
   pat->ch[c][s]=empty_step(c);
   /* V19 default CH5 instrument was 0. Keep old projects sonically identical. */
   if(rd16(data+4u)<20u && c==4u)pat->ch[c][s].instrument=0u;
  }
  uint32_t bit=0u,packed=0u;uint8_t present[TRACK_V5_MASK_BYTES];
  if(v5){
   if(i+TRACK_V5_MASK_BYTES>n)return -2;
   memcpy(present,data+i,TRACK_V5_MASK_BYTES);i+=TRACK_V5_MASK_BYTES;
   uint32_t count=0u;
   for(uint8_t x=0u;x<TRACK_V5_MASK_BYTES;x++)for(uint8_t b=0u;b<8u;b++)count+=(present[x]>>b)&1u;
   packed=(count*(rd16(data+4u)>=20u?TRACK_V20_PACKED_STEP_BITS:rd16(data+4u)>=12u?TRACK_V12_PACKED_STEP_BITS:rd16(data+4u)>=11u?TRACK_V11_PACKED_STEP_BITS:TRACK_PACKED_STEP_BITS)+7u)/8u;
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
    st->instrument=(uint8_t)unpack_bits(data+i,&bit,rd16(data+4u)>=20u?3u:2u);
    st->chord=(uint8_t)unpack_bits(data+i,&bit,4u);
    st->effect=(uint8_t)unpack_bits(data+i,&bit,4u);
    st->value=(uint8_t)unpack_bits(data+i,&bit,7u);
    st->dynamics=rd16(data+4u)>=11u?(uint8_t)unpack_bits(data+i,&bit,8u):0u;
    st->groove=rd16(data+4u)>=12u?(uint8_t)unpack_bits(data+i,&bit,7u):0u;
   }else{
    GET1(st->note);GET1(st->instrument);GET1(st->chord);GET1(st->effect);GET1(st->value);
   }
   if(!valid_step(st,lane))return -3;
  }
  if(rd16(data+4u)>=4u)i+=packed;
  for(s=0;s<TRACK_STEPS;s++){pat->note2_fx[s].effect=0u;pat->note2_fx[s].value=50u;}
  if(rd16(data+4u)>=18u){
   if(i+2u>n)return -2;
   uint16_t mask=rd16(data+i);i+=2u;
   for(s=0;s<TRACK_STEPS;s++)if(mask&(1u<<s)){
    GET1(pat->note2_fx[s].effect);GET1(pat->note2_fx[s].value);
    if(pat->note2_fx[s].effect>=TRACK_FX_COUNT||pat->note2_fx[s].value>100u)return -3;
   }
  }
  /* Decode sparse per-pattern FX entries into the shared bank. */
  if(rd16(data+4u)>=21u){
   uint8_t count;GET1(count);
   if(count>TRACK_EXTRA_FX_CAP || p->song.extras.count+count>TRACK_EXTRA_FX_CAP)return -3;
   for(uint8_t x=0u;x<count;x++){
    uint8_t key,fx,value;GET1(key);GET1(fx);GET1(value);
    uint8_t lane=(uint8_t)((key&127u)/16u),row=(uint8_t)((key&127u)%16u);
    if(lane>=TRACK_CHANNELS||((key&128u)!=0u&&lane!=TRACK_DRUM_CHANNEL)||
       !(fx&127u)||(fx&127u)>=TRACK_FX_COUNT||value>100u||
       track_extra_count(&p->song,r,key)>=2u)return -3;
    uint8_t slot=(uint8_t)(rd16(data+4u)>=22u?((fx>>7u)&1u):track_extra_count(&p->song,r,key));
    if(track_extra_get(&p->song,r,key,slot))return -3;
    p->song.extras.data[p->song.extras.count++]=(struct track_extra_fx){r,key,(uint8_t)((fx&127u)|(slot<<7u)),value};
   }
  }
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
  for(c=0;c<(rd16(data+4u)>=20u?TRACK_CHIP_SLOTS:4u);c++){
   struct track_store_instrument *v=&p->instruments[c];
   GET1(v->type);GET1(v->attack);GET1(v->decay);GET1(v->sustain);GET1(v->release);
   GET1(v->duty);GET1(v->volume);if(i+2u>n)return -2;
  v->cutoff=rd16(data+i);i+=2;
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
  for(c=0;c<(rd16(data+4u)>=20u?TRACK_CHIP_SLOTS:4u);c++){
   struct track_store_instrument *v=&p->instruments[c];
   v->type=c;v->attack=1u;v->decay=14u;v->sustain=100u;v->release=16u;
   v->duty=50u;v->cutoff=7000u;v->volume=65u;track_va_patch_default(&v->va);
  }
 }
 if(rd16(data+4u)<20u){
  /* Migration: legacy 1-4 remain untouched; 5-8 are independent factory sounds. */
  for(c=4u;c<TRACK_CHIP_SLOTS;c++){
   struct track_store_instrument *v=&p->instruments[c];
   v->type=(uint8_t)(c==4u?TRACK_CHIP_VA_OSC:c==5u?TRACK_CHIP_VA_OSC2:c==6u?TRACK_CHIP_SID_6581:TRACK_CHIP_NES_PULSE);
   v->attack=(uint8_t)(c==5u?18u:1u);
   v->decay=(uint8_t)(c==4u?25u:c==5u?32u:14u);
   v->sustain=(uint8_t)(c==4u?70u:c==5u?80u:100u);
   v->release=(uint8_t)(c==4u?18u:c==5u?48u:16u);
   v->duty=50u;v->cutoff=(uint16_t)(c==4u?3200u:c==5u?8400u:7000u);v->volume=65u;
   track_va_patch_factory(c,&v->va);
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
  p->punch_param[1]=20u;p->punch_param[2]=0u;p->punch_param[4]=50u;
 }
 if(rd16(data+4u)>=10u){
  GET1(p->master.comp_attack);GET1(p->master.reverb_spread);
  if(p->master.comp_attack>100u||p->master.reverb_spread>100u)return -3;
 }else{p->master.comp_attack=28u;p->master.reverb_spread=50u;}
 if(rd16(data+4u)>=13u){GET1(p->mixer.quantize_mutes);if(p->mixer.quantize_mutes>1u)return -3;}
 else p->mixer.quantize_mutes=0u; /* Older projects had no quantized-mute preference; default immediate */
 if(rd16(data+4u)>=14u){
  GET1(p->snapshots.valid);
  if(p->snapshots.valid & (uint8_t)~0x0Fu)return -3;
  for(c=0u;c<BEETLE_SNAPSHOT_SLOTS;c++){
   GET1(p->snapshots.masks[c]);
   if(p->snapshots.masks[c] & (uint8_t)~BEETLE_SNAPSHOT_MASK)return -3;
  }
 }else beetle_snapshots_init(&p->snapshots);
 if(rd16(data+4u)>=15u){
  for(c=0u;c<BEETLE_GROUP_SLOTS;c++){
   GET1(p->groups.masks[c]);
   if(p->groups.masks[c] & (uint8_t)~BEETLE_GROUP_MASK)return -3;
  }
 }else beetle_groups_init(&p->groups);

 if(rd16(data+4u)>=16u){
  GET1(p->library.valid);
  for(c=0u;c<BEETLE_LIBRARY_SLOTS;c++){
   if(!(p->library.valid&(1u<<c)))continue;
   for(uint8_t j=0u;j<BEETLE_LIBRARY_NAME;j++){
    uint8_t ch;GET1(ch);
    if(!((ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9')||ch==' '||ch=='-'||ch=='_'))return -3;
    p->library.name[c][j]=(char)ch;
   }
   struct track_store_instrument *v=&p->library.patch[c];
   GET1(v->type);GET1(v->attack);GET1(v->decay);GET1(v->sustain);GET1(v->release);
   GET1(v->duty);GET1(v->volume);
   if(i+2u>n)return -2;
  v->cutoff=rd16(data+i);i+=2;
   GET1(v->va.wave_a);GET1(v->va.wave_b);GET1(s);v->va.coarse=(int8_t)s;
   GET1(s);v->va.fine=(int8_t)s;GET1(v->va.detune);GET1(v->va.mix);
   GET1(v->va.pulse_width);GET1(v->va.morph);
   GET1(s);v->va.coarse_b=(int8_t)s;GET1(s);v->va.fine_b=(int8_t)s;
   GET1(v->va.pulse_width_b);GET1(v->va.drive);GET1(v->va.sub);
   GET1(v->va.noise);GET1(v->va.fm_depth);
   if(v->type>=TRACK_CHIP_TYPES||v->duty>100u||v->volume>100u||!track_va_patch_valid(&v->va))return -3;
  }
 }
 /* V16 and earlier migrate by seeding each active waveform. */
 if(rd16(data+4u)>=17u){
  for(c=0u;c<(rd16(data+4u)>=20u?TRACK_CHIP_SLOTS:4u);c++){
   if(i+2u>n)return -2;
  p->type_valid[c]=rd16(data+i);i+=2;
   if(p->type_valid[c] & (uint16_t)~((1u<<TRACK_CHIP_TYPES)-1u))return -3;
   for(uint8_t t=0u;t<TRACK_CHIP_TYPES;t++)if(p->type_valid[c]&(1u<<t)){
    struct track_store_instrument *v=&p->type_presets[c][t];
    GET1(v->type);GET1(v->attack);GET1(v->decay);GET1(v->sustain);GET1(v->release);
    GET1(v->duty);GET1(v->volume);
    if(i+2u>n)return -2;
  v->cutoff=rd16(data+i);i+=2;
    GET1(v->va.wave_a);GET1(v->va.wave_b);GET1(s);v->va.coarse=(int8_t)s;
    GET1(s);v->va.fine=(int8_t)s;GET1(v->va.detune);GET1(v->va.mix);
    GET1(v->va.pulse_width);GET1(v->va.morph);
    GET1(s);v->va.coarse_b=(int8_t)s;GET1(s);v->va.fine_b=(int8_t)s;
    GET1(v->va.pulse_width_b);GET1(v->va.drive);GET1(v->va.sub);
    GET1(v->va.noise);GET1(v->va.fm_depth);
    if(v->type!=t || v->duty>100u||v->volume>100u||
       v->attack>100u||v->decay>100u||v->sustain>100u||v->release>100u||
       !track_va_patch_valid(&v->va))return -3;
   }
  }
 }else{
  for(c=0u;c<4u;c++){
   uint8_t t=p->instruments[c].type;
   if(t<TRACK_CHIP_TYPES){p->type_valid[c]=(uint16_t)(1u<<t);p->type_presets[c][t]=p->instruments[c];}
  }
 }
 if(rd16(data+4u)<20u){
  for(c=4u;c<TRACK_CHIP_SLOTS;c++){
   uint8_t t=p->instruments[c].type;
   p->type_valid[c]=(uint16_t)(1u<<t);p->type_presets[c][t]=p->instruments[c];
  }
 }
 if(rd16(data+4u)>=23u && i<n){
  uint32_t used=0u;
  if(i+BEETLE_ONESHOT_NOTES*10u+6u>n)return -2;
  uint32_t start=i;
  for(uint8_t x=0u;x<BEETLE_ONESHOT_NOTES;x++){
   p->oneshots.length[x]=rd16(data+i);i+=2u;used+=p->oneshots.length[x];
   if(used>BEETLE_ONESHOT_MAX_BYTES)return -3;
   for(uint8_t y=0u;y<8u;y++){
    uint8_t ch=data[i++];
    if(ch && !(ch>='A'&&ch<='Z') && !(ch>='0'&&ch<='9') && ch!=' '&&ch!='-'&&ch!='_')return -3;
    p->oneshots.name[x][y]=(char)ch;
   }
  }
  if(i+used+6u!=n)return -3;
  if(used && (!p->oneshots.first || (used>BEETLE_ONESHOT_FIRST_BYTES&&!p->oneshots.second)))return -3;
  for(uint32_t x=0u;x<used;x++)beetle_oneshot_set(&p->oneshots,x,data[i++]);
  if(rd16(data+i)!=(uint16_t)start)return -3;i+=2u;
  if(data[i++]!='B'||data[i++]!='1'||data[i++]!='O'||data[i++]!='S')return -3;
 }
 if(i!=n)return -3;
 /* Upgrading an old sparse V19 pattern must not materialize 768 empty
  * CH5 cells. Empty rows never trigger an oscillator, so translate just
  * the truly default rows to the new fifth synth lane's patch slot 5. */
 if(rd16(data+4u)<20u){
  for(uint8_t r0=0u;r0<TRACK_PATTERNS;r0++){
   struct track_pattern *pat=&p->song.patterns[r0];
   for(uint8_t step0=0u;step0<TRACK_STEPS;step0++){
    struct track_step *v=&pat->ch[4u][step0];
    if(v->note==TRACK_NOTE_EMPTY && v->instrument==0u &&
       v->chord==0u && v->effect==0u && v->value==50u &&
       v->dynamics==0u && v->groove==0u)v->instrument=4u;
   }
  }
 }
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
