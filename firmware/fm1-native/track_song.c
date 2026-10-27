/* SPDX-License-Identifier: GPL-3.0-only */
#include "track_song.h"
#include "track_drum.h"
#include <string.h>
static int32_t bound(int32_t n,int32_t lo,int32_t hi){return n<lo?lo:n>hi?hi:n;}
void track_song_init(struct track_song_bank *b,const struct track_engine *e){
 if(!b||!e)return;
 memset(b,0,sizeof(*b));
 for(uint8_t i=0;i<TRACK_PATTERNS;i++){
   b->patterns[i].length=16u;
   for(uint8_t s=0;s<TRACK_STEPS;s++){b->patterns[i].note2_fx[s].value=50u;b->patterns[i].note3[s]=TRACK_NOTE_EMPTY;}
  for(uint8_t c=0;c<TRACK_CHANNELS;c++)for(uint8_t s=0;s<TRACK_STEPS;s++){
   b->patterns[i].ch[c][s].note=255u;
   b->patterns[i].ch[c][s].instrument=(uint8_t)(c==TRACK_DRUM_CHANNEL?3u:c);
   b->patterns[i].ch[c][s].value=50u;
  }
 }
 b->patterns[0]=e->active;
 b->chain[0].pattern=0u;b->chain[0].repeats=1u;
 for(uint8_t c=0;c<TRACK_CHANNELS;c++)b->chain[0].lane[c]=TRACK_SONG_AUTO_BASE;
 b->count=1u;b->loop=1u;
}
static uint8_t track_song_auto_has_content(const struct track_song_bank *b,uint8_t source,uint8_t channel){
 if(!b||source>=TRACK_PATTERNS||channel>=TRACK_CHANNELS)return 0u;
 if(track_lane_has_data(&b->patterns[source],channel))return 1u;
 if(channel==TRACK_DRUM_CHANNEL){
  for(uint8_t step=0u;step<TRACK_STEPS;step++)
   if(b->patterns[source].note2_fx[step].effect!=0u)return 1u;
 }
 for(uint8_t i=0u;i<b->extras.count;i++){
  const struct track_extra_fx *f=&b->extras.data[i];
  if(f->pattern!=source)continue;
  uint8_t key=(uint8_t)(f->id&127u);
  if((uint8_t)(key/TRACK_STEPS)==channel)return 1u;
 }
 return 0u;
}
uint8_t track_song_lane_visible(const struct track_song_bank *b,uint8_t encoded,uint8_t channel){
 if(!track_song_valid_lane(encoded)||encoded==TRACK_SONG_SILENT)return 0u;
 uint8_t src=track_song_source(encoded);
 if(encoded<TRACK_PATTERNS)return 1u; /* explicitly selected EMPTY pattern sustains */
 return track_song_auto_has_content(b,src,channel);
}
uint8_t track_song_stop_mask(const struct track_song_bank *b){
 if(!b||!b->enabled||!b->composite_active||b->position>=b->count)return 0u;
 uint8_t stop=0u;
 for(uint8_t ch=0u;ch<TRACK_CHANNELS;ch++)
  if(!track_song_lane_visible(b,b->chain[b->position].lane[ch],ch))stop|=(uint8_t)(1u<<ch);
 return stop;
}
void track_song_capture(struct track_song_bank *b,const struct track_engine *e){
 if(!b||!e||b->composite_active||e->pattern_slot>=TRACK_PATTERNS)return;
 b->patterns[e->pattern_slot]=e->active;
 track_extra_prune(b,e->pattern_slot,&e->active);
}
int track_song_switch(struct track_song_bank *b,struct track_engine *e,uint8_t index){
 if(!b||!e||index>=TRACK_PATTERNS)return -1;
 if(b->composite_active){b->composite_active=0u;}else track_song_capture(b,e);
 e->active=b->patterns[index];e->pattern_slot=index;
 /* Different patterns may have different lengths. Never leave focus on an
  * invisible step after manual, queued, or song-controlled switching. */
 uint8_t len=e->active.length>=1u&&e->active.length<=TRACK_STEPS?e->active.length:TRACK_STEPS;
 if(e->transport.edit_step>=len)e->transport.edit_step=(uint8_t)(len-1u);
 return 0;
}
/* Song compositions are playback-only. Never capture them into source patterns.
 * This function runs at song boundaries, never during USB flash writes. */
static void track_song_compose(struct track_song_bank *b,struct track_engine *e){
 const struct track_song_entry *ent=&b->chain[b->position];
 struct track_pattern *out=&e->active;
 uint8_t length=1u;
 for(uint8_t c=0u;c<TRACK_CHANNELS;c++){
  uint8_t src=track_song_source(ent->lane[c]);
  if(src<TRACK_PATTERNS && track_song_lane_visible(b,ent->lane[c],c)){
   uint8_t plen=b->patterns[src].length;
   if(plen>length&&plen<=TRACK_STEPS)length=plen;
  }
 }
 for(uint8_t c=0u;c<TRACK_CHANNELS;c++){
  uint8_t src=track_song_source(ent->lane[c]);
  const struct track_pattern *p=track_song_lane_visible(b,ent->lane[c],c)?&b->patterns[src]:0;
  uint8_t plen=p&&p->length&&p->length<=TRACK_STEPS?p->length:TRACK_STEPS;
  if(p){
   /* Most patterns are 16 steps. One contiguous bounded copy replaces
    * 16 per-cell modulo operations at each SONG boundary in the audio ISR. */
   if(plen==TRACK_STEPS){
    memcpy(out->ch[c],p->ch[c],sizeof(out->ch[c]));
    if(c==TRACK_DRUM_CHANNEL){memcpy(out->note2_fx,p->note2_fx,sizeof(out->note2_fx));memcpy(out->note3,p->note3,sizeof(out->note3));}
   }else{
    for(uint8_t s=0u;s<TRACK_STEPS;s++){
     uint8_t k=(uint8_t)(s%plen);
     out->ch[c][s]=p->ch[c][k];
     if(c==TRACK_DRUM_CHANNEL){out->note2_fx[s]=p->note2_fx[k];out->note3[s]=p->note3[k];}
    }
   }
  }else{
   for(uint8_t s=0u;s<TRACK_STEPS;s++){
    struct track_step *dst=&out->ch[c][s];
    memset(dst,0,sizeof(*dst));dst->note=TRACK_NOTE_EMPTY;
    dst->instrument=(uint8_t)(c==TRACK_DRUM_CHANNEL?3u:c);dst->value=50u;
    if(c==TRACK_DRUM_CHANNEL){out->note2_fx[s].effect=0u;out->note2_fx[s].value=50u;out->note3[s]=TRACK_NOTE_EMPTY;}
   }
  }
 }
 /* Extra FX are looked up from each source lane while Song Mode is active.
  * Do not merge several patterns into a limited 16-entry scratch pool. */
 out->length=length;
 if(e->transport.edit_step>=length)e->transport.edit_step=(uint8_t)(length-1u);
}
void track_song_restore(struct track_song_bank *b,struct track_engine *e){
 if(!b||!e)return;
 if(b->composite_active && e->pattern_slot<TRACK_PATTERNS)
  e->active=b->patterns[e->pattern_slot];
 b->composite_active=0u;
 b->play_range_active=0u; /* never carry a PLAY selection into a later transport start */
}
uint8_t track_song_punch_pattern(const struct track_song_bank *b,const struct track_engine *e){
 if(b&&b->enabled&&b->composite_active&&b->position<b->count){
  uint8_t source=track_song_source(b->chain[b->position].lane[0]);
  return source<TRACK_PATTERNS?source:TRACK_PATTERNS;
 }
 return e?e->pattern_slot:TRACK_PATTERNS;
}
void track_song_start_range(struct track_song_bank *b,struct track_engine *e,
                            uint8_t first,uint8_t last){
 if(!b||!e||!b->enabled||!b->count)return;
 track_song_restore(b,e);
 track_song_capture(b,e);
 if(first>last){uint8_t tmp=first;first=last;last=tmp;}
 if(first>=b->count)first=(uint8_t)(b->count-1u);
 if(last>=b->count)last=(uint8_t)(b->count-1u);
 b->play_range_first=first;
 b->play_range_last=last;
 b->play_range_active=1u;
 b->position=first;b->repeat_index=0u;b->composite_active=1u;
 track_song_compose(b,e);
}
void track_song_start(struct track_song_bank *b,struct track_engine *e){
 if(!b||!e||!b->enabled||!b->count)return;
 track_song_start_range(b,e,0u,(uint8_t)(b->count-1u));
 b->play_range_active=0u; /* regular full-song transport keeps global LOOP */
}
int track_song_on_wrap(struct track_song_bank *b,struct track_engine *e){
 if(!b||!e||!b->enabled||!b->count)return 0;
 if(b->position>=b->count)return -1;
 const struct track_song_entry *entry=&b->chain[b->position];
 if(b->repeat_index+1u<entry->repeats){b->repeat_index++;return 0;}
 b->repeat_index=0u;
 /* Selection always takes precedence over the global song loop setting. */
 uint8_t last=b->play_range_active?b->play_range_last:(uint8_t)(b->count-1u);
 if(last>=b->count)last=(uint8_t)(b->count-1u);
 if(b->position>=last){
  if(!b->play_range_active && !b->loop){
   track_stop(e);track_song_restore(b,e);return -1;
  }
  b->position=b->play_range_active?b->play_range_first:0u;
  if(b->position>=b->count)b->position=0u;
 }else b->position++;
 track_song_compose(b,e);return 1;
}
int track_song_insert(struct track_song_bank *b,uint8_t after){
 if(!b||after>=b->count||b->count>=TRACK_SONG_ENTRIES)return -1;
 for(uint8_t i=b->count;i>after+1u;i--)b->chain[i]=b->chain[i-1u];
 /* A new row is an exact copy of the previous row: lane references,
  * automatic/manual binding state and repeats all survive insertion. */
 uint8_t row=(uint8_t)(after+1u);
 b->chain[row]=b->chain[after];
 b->count++;b->edit_row=row;return 0;
}
int track_song_delete(struct track_song_bank *b,uint8_t at){
 if(!b||b->count<=1u||at>=b->count)return -1;
 for(uint8_t i=at;i+1u<b->count;i++)b->chain[i]=b->chain[i+1u];
 b->count--;if(b->edit_row>=b->count)b->edit_row=b->count-1u;
 if(b->position>=b->count)b->position=0u;
 return 0;
}
int track_song_change(struct track_song_bank *b,uint8_t row,uint8_t col,int32_t delta){
 if(!b||row>=b->count||col>6u||!delta)return -1;
 struct track_song_entry *e=&b->chain[row];
 if(col<TRACK_CHANNELS){
  /* -1 (displayed --) is the silence sentinel; -1 detent from P01. */
  int32_t current=e->lane[col]==TRACK_SONG_SILENT?-1:track_song_source(e->lane[col]);
  int32_t next=bound(current+delta,-1,TRACK_PATTERNS-1);
  e->lane[col]=next<0?TRACK_SONG_SILENT:(uint8_t)next;
 }else e->repeats=(uint8_t)bound((int32_t)e->repeats+delta,1,16);
 return 0;
}
int track_song_record_value(struct track_song_bank *b,uint8_t pat,uint8_t step,uint8_t fx,uint8_t down,uint8_t value){
 if(!b||pat>=TRACK_PATTERNS||step>=TRACK_STEPS||fx>=TRACK_PUNCH_COUNT||down>2u||
     (value>100u&&value!=255u))return -1;
 struct track_song_clip *clip=&b->clips[pat];
 /* At 16-step resolution keep only the latest encoder position for each
  * effect in each row. Prevent rapid turns from exhausting the event bank. */
 if(down==2u){
  for(int i=(int)clip->count-1;i>=0;i--){
   struct track_song_event *ev=&clip->data[i];
   if(ev->step==step&&ev->fx==fx&&ev->down==2u){ev->param=value;return 0;}
  }
 }
 if(clip->count>=TRACK_SONG_FX_EVENTS){clip->overflow=1u;return -2;}
 /* A punch shorter than one sequencer step must still be visible and
  * replayable: move its STP boundary to the next step, rather than
  * recording ON and OFF at the same timestamp and cancelling both. */
 if(down==0u){
  for(int i=(int)clip->count-1;i>=0;i--){
   if(clip->data[i].fx==fx){
    if(clip->data[i].down && clip->data[i].step==step)
     step=(uint8_t)((step+1u)%TRACK_STEPS);
    break;
   }
  }
 }
 clip->data[clip->count].step=step;
 clip->data[clip->count].fx=fx;
 clip->data[clip->count].down=down;
 clip->data[clip->count].param=value;
 clip->count++;return 0;
}
int track_song_record(struct track_song_bank *b,uint8_t pat,uint8_t step,uint8_t fx,uint8_t down){
 return track_song_record_value(b,pat,step,fx,down,255u);
}
uint8_t track_song_effect_parameter(const struct track_song_bank *b,uint8_t pat,uint8_t step,uint8_t fx,uint8_t fallback){
 if(!b||pat>=TRACK_PATTERNS||step>=TRACK_STEPS||fx>=TRACK_PUNCH_COUNT)return fallback;
 const struct track_song_clip *clip=&b->clips[pat];
 int best=-1,best_time=-1;
 for(uint8_t i=0u;i<clip->count;i++){
  const struct track_song_event *e=&clip->data[i];
  if(e->fx!=fx||e->step>step||e->down==0u||e->param>100u)continue;
  if((int)e->step>=best_time){best_time=e->step;best=i;}
 }
 if(best<0)for(uint8_t i=0u;i<clip->count;i++){
  const struct track_song_event *e=&clip->data[i];
  if(e->fx!=fx||e->down==0u||e->param>100u)continue;
  if((int)e->step>=best_time){best_time=e->step;best=i;}
 }
 return best<0?fallback:clip->data[best].param;
}
uint16_t track_song_effect_mask_boundary(const struct track_song_bank *b,uint8_t pat,uint8_t step,uint8_t allow_wrap){
 if(!b||pat>=TRACK_PATTERNS||step>=TRACK_STEPS)return 0u;
 const struct track_song_clip *clip=&b->clips[pat];uint16_t mask=0u;
 /* Evaluate each Punch lane as a periodic timeline. Record order only wins
  * ties on exactly the same step. This correctly handles a short hit on
  * step 15 whose STP falls on step 00 of the next loop. */
 for(uint8_t fx=0;fx<TRACK_PUNCH_COUNT;fx++){
  int best=-1;int best_time=-1;
  for(uint8_t i=0;i<clip->count;i++){
   const struct track_song_event *ev=&clip->data[i];
   if(ev->fx!=fx||ev->step>step||ev->down>1u)continue;
   if((int)ev->step>=best_time){best_time=ev->step;best=i;}
  }
  if(best<0 && allow_wrap){
   /* During an ordinary repeating pattern, a genuine held FX may cross the wrap.
    * At transport start or a different Song row, there is NO prior state. */
   for(uint8_t i=0;i<clip->count;i++){
    const struct track_song_event *ev=&clip->data[i];
    if(ev->fx!=fx||ev->down>1u)continue;
    if((int)ev->step>=best_time){best_time=ev->step;best=i;}
   }
  }
  if(best>=0&&clip->data[best].down)mask|=(uint16_t)(1u<<fx);
 }
 return mask;
}

uint16_t track_song_effect_mask(const struct track_song_bank *b,uint8_t pat,uint8_t step){
 return track_song_effect_mask_boundary(b,pat,step,1u);
}

/* A new take replaces only this effect lane; do not mix its fresh down/up
 * edges with stale events from an earlier take, which caused ghost FX. */
void track_song_clear_punch_fx(struct track_song_bank *b,uint8_t pat,uint8_t fx){
 if(!b||pat>=TRACK_PATTERNS||fx>=TRACK_PUNCH_COUNT)return;
 struct track_song_clip *clip=&b->clips[pat];uint8_t w=0u;
 for(uint8_t i=0u;i<clip->count;i++)if(clip->data[i].fx!=fx)
  clip->data[w++]=clip->data[i];
 clip->count=w;clip->overflow=0u;
}
