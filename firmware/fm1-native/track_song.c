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
   for(uint8_t s=0;s<TRACK_STEPS;s++)b->patterns[i].note2_fx[s].value=50u;
  for(uint8_t c=0;c<TRACK_CHANNELS;c++)for(uint8_t s=0;s<TRACK_STEPS;s++){
   b->patterns[i].ch[c][s].note=255u;
   b->patterns[i].ch[c][s].instrument=(uint8_t)(c==TRACK_DRUM_CHANNEL?3u:(c&3u));
   b->patterns[i].ch[c][s].value=50u;
  }
 }
 b->patterns[0]=e->active;
 b->chain[0].pattern=0u;b->chain[0].repeats=1u;
 for(uint8_t c=0;c<TRACK_CHANNELS;c++)b->chain[0].lane[c]=0u;
 b->count=1u;b->loop=1u;
}
void track_song_capture(struct track_song_bank *b,const struct track_engine *e){
 if(!b||!e||b->composite_active||e->pattern_slot>=TRACK_PATTERNS)return;
 b->patterns[e->pattern_slot]=e->active;
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
  uint8_t src=ent->lane[c];
  if(src<TRACK_PATTERNS){
   uint8_t plen=b->patterns[src].length;
   if(plen>length&&plen<=TRACK_STEPS)length=plen;
  }
 }
 for(uint8_t c=0u;c<TRACK_CHANNELS;c++){
  uint8_t src=ent->lane[c];
  const struct track_pattern *p=src<TRACK_PATTERNS?&b->patterns[src]:0;
  uint8_t plen=p&&p->length&&p->length<=TRACK_STEPS?p->length:TRACK_STEPS;
  for(uint8_t s=0u;s<TRACK_STEPS;s++){
   struct track_step *dst=&out->ch[c][s];
   if(p)*dst=p->ch[c][s%plen];
   else {memset(dst,0,sizeof(*dst));dst->note=TRACK_NOTE_EMPTY;
    dst->instrument=(uint8_t)(c==TRACK_DRUM_CHANNEL?3u:(c&3u));dst->value=50u;}
   if(c==TRACK_DRUM_CHANNEL){
    if(p)out->note2_fx[s]=p->note2_fx[s%plen];
    else {out->note2_fx[s].effect=0u;out->note2_fx[s].value=50u;}
   }
  }
 }
 out->length=length;
 if(e->transport.edit_step>=length)e->transport.edit_step=(uint8_t)(length-1u);
}
void track_song_restore(struct track_song_bank *b,struct track_engine *e){
 if(!b||!e||!b->composite_active)return;
 if(e->pattern_slot<TRACK_PATTERNS)e->active=b->patterns[e->pattern_slot];
 b->composite_active=0u;
}
uint8_t track_song_punch_pattern(const struct track_song_bank *b,const struct track_engine *e){
 if(b&&b->enabled&&b->composite_active&&b->position<b->count){
  uint8_t source=b->chain[b->position].lane[0];
  return source<TRACK_PATTERNS?source:TRACK_PATTERNS;
 }
 return e?e->pattern_slot:TRACK_PATTERNS;
}
void track_song_start(struct track_song_bank *b,struct track_engine *e){
 if(!b||!e||!b->enabled||!b->count)return;
 track_song_restore(b,e);
 track_song_capture(b,e);
 b->position=0u;b->repeat_index=0u;b->composite_active=1u;
 track_song_compose(b,e);
}
int track_song_on_wrap(struct track_song_bank *b,struct track_engine *e){
 if(!b||!e||!b->enabled||!b->count)return 0;
 if(b->position>=b->count)return -1;
 const struct track_song_entry *entry=&b->chain[b->position];
 if(b->repeat_index+1u<entry->repeats){b->repeat_index++;return 0;}
 b->repeat_index=0u;
 if(b->position+1u>=b->count){
  if(!b->loop){track_stop(e);track_song_restore(b,e);return -1;}
  b->position=0u;
 }else b->position++;
 track_song_compose(b,e);return 1;
}
int track_song_insert(struct track_song_bank *b,uint8_t after){
 if(!b||after>=b->count||b->count>=TRACK_SONG_ENTRIES)return -1;
 for(uint8_t i=b->count;i>after+1u;i--)b->chain[i]=b->chain[i-1u];
 /* New row defaults to the pattern numbered by its displayed position.
  * Existing rows are moved intact (their manually edited chains stay intact). */
 uint8_t row=(uint8_t)(after+1u);
 uint8_t pat=(uint8_t)(row<TRACK_PATTERNS?row:TRACK_PATTERNS-1u);
 b->chain[row].pattern=pat;b->chain[row].repeats=1u;
 for(uint8_t c=0u;c<TRACK_CHANNELS;c++)b->chain[row].lane[c]=pat;
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
  int32_t current=e->lane[col]==TRACK_SONG_SILENT?-1:e->lane[col];
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
 if(!b||pat>=TRACK_PATTERNS||step>=TRACK_STEPS||fx>=5u)return fallback;
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
uint8_t track_song_effect_mask(const struct track_song_bank *b,uint8_t pat,uint8_t step){
 if(!b||pat>=TRACK_PATTERNS||step>=TRACK_STEPS)return 0u;
 const struct track_song_clip *clip=&b->clips[pat];uint8_t mask=0u;
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
  if(best<0){
   /* Carry a long-held Punch through the next pattern wrap. */
   for(uint8_t i=0;i<clip->count;i++){
    const struct track_song_event *ev=&clip->data[i];
    if(ev->fx!=fx||ev->down>1u)continue;
    if((int)ev->step>=best_time){best_time=ev->step;best=i;}
   }
  }
  if(best>=0&&clip->data[best].down)mask|=(uint8_t)(1u<<fx);
 }
 return mask;
}
