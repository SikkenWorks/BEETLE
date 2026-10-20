from pathlib import Path
root=Path('/mnt/data/beetle_v134_final')
def patch(name,old,new,count=1):
 p=root/name;s=p.read_text();n=s.count(old)
 if n!=count: raise Exception(f'{name} expected {count}, got {n}: {old[:90]}')
 p.write_text(s.replace(old,new))
base='firmware/fm1-native/'
patch(base+'track_core.h', '#define TRACK_FX_COUNT 12u', '#define TRACK_FX_COUNT 12u\n#define TRACK_EXTRA_FX_CAP 16u /* up to sixteen secondary FX per pattern, two per step */')
patch(base+'track_core.h','struct track_pattern {struct track_step ch[TRACK_CHANNELS][TRACK_STEPS];uint8_t length;\n struct track_note2_fx note2_fx[TRACK_STEPS];};', '''/* Sparse secondary FX: id = lane*16+step (0..95); bit7 for CH6 NOTE2.
 * Two entries at most for each id; insertion order is FX2 then FX3.
 * A full pool must fail atomically and show a UI warning. */
struct track_extra_fx {uint8_t id,effect,value;};
struct track_pattern {struct track_step ch[TRACK_CHANNELS][TRACK_STEPS];uint8_t length;
 struct track_note2_fx note2_fx[TRACK_STEPS];
 uint8_t extra_count;struct track_extra_fx extra[TRACK_EXTRA_FX_CAP];};
static inline uint8_t track_extra_key(uint8_t lane,uint8_t row,uint8_t note2){
 return (uint8_t)(lane*TRACK_STEPS+row+(note2?128u:0u));
}
static inline uint8_t track_extra_count(const struct track_pattern *p,uint8_t key){
 uint8_t n=0u;if(!p)return 0u;
 for(uint8_t i=0u;i<p->extra_count&&i<TRACK_EXTRA_FX_CAP;i++)if(p->extra[i].id==key)n++;
 return n;
}
static inline struct track_extra_fx *track_extra_get(struct track_pattern *p,uint8_t key,uint8_t slot){
 if(!p||slot>=2u)return 0;
 for(uint8_t i=0u;i<p->extra_count&&i<TRACK_EXTRA_FX_CAP;i++)if(p->extra[i].id==key){
  if(!slot)return &p->extra[i];slot--;
 }
 return 0;
}
static inline const struct track_extra_fx *track_extra_find(const struct track_pattern *p,uint8_t key,uint8_t slot){
 return track_extra_get((struct track_pattern*)p,key,slot);
}
/* Returns 1=created, 0=already present, -1=full, -2=gap/invalid. */
static inline int track_extra_add(struct track_pattern *p,uint8_t key,uint8_t slot,uint8_t fx,uint8_t value){
 if(!p||slot>=2u||fx==0u||fx>=TRACK_FX_COUNT||value>100u)return -2;
 if(track_extra_get(p,key,slot))return 0;
 if(track_extra_count(p,key)!=slot)return -2;
 if(p->extra_count>=TRACK_EXTRA_FX_CAP)return -1;
 p->extra[p->extra_count++]=(struct track_extra_fx){key,fx,value};return 1;
}
static inline void track_extra_remove(struct track_pattern *p,uint8_t key,uint8_t slot){
 if(!p||slot>=2u)return;
 for(uint8_t i=0u,matched=0u;i<p->extra_count;i++)if(p->extra[i].id==key){
  if(matched++==slot){for(uint8_t j=i;j+1u<p->extra_count;j++)p->extra[j]=p->extra[j+1u];
   p->extra_count--;return;}
 }
}
static inline void track_extra_clear(struct track_pattern *p,uint8_t key){
 while(track_extra_count(p,key))track_extra_remove(p,key,0u);
}''')
patch(base+'track_core.c',' s->chord=0u;s->effect=0u;s->value=50u;s->dynamics=0u;s->groove=0u;e->dirty=1u;return 0;', ' s->chord=0u;s->effect=0u;s->value=50u;s->dynamics=0u;s->groove=0u;\n track_extra_clear(&e->active,track_extra_key(channel,step,0u));\n if(channel==TRACK_DRUM_CHANNEL)track_extra_clear(&e->active,track_extra_key(channel,step,1u));\n e->dirty=1u;return 0;')
patch(base+'track_song.c',' out->length=length;\n if(e->transport.edit_step>=length)', ' /* Extra FX are looked up from each source lane while Song Mode is active.\n  * Do not merge several patterns into a limited 16-entry scratch pool. */\n out->extra_count=0u;\n out->length=length;\n if(e->transport.edit_step>=length)')
# persistence after note2 block before punch count; backwards V20 unchanged parse
patch(base+'track_project_store.h','#define TRACK_STORE_VERSION 20u','#define TRACK_STORE_VERSION 21u')
patch(base+'track_project_store.c','  const struct track_pattern *pat=&p->song.patterns[k];\n  if(pat->length<1u || pat->length>TRACK_STEPS)return 0u;', '''  const struct track_pattern *pat=&p->song.patterns[k];
  if(pat->length<1u || pat->length>TRACK_STEPS||pat->extra_count>TRACK_EXTRA_FX_CAP)return 0u;
  for(uint8_t x=0u;x<pat->extra_count;x++){
   const struct track_extra_fx *f=&pat->extra[x];uint8_t key=f->id;
   uint8_t lane=(uint8_t)((key&127u)/TRACK_STEPS),row=(uint8_t)(key%TRACK_STEPS);
   if(lane>=TRACK_CHANNELS||((key&128u)!=0u&&lane!=TRACK_DRUM_CHANNEL)||
      f->effect==0u||f->effect>=TRACK_FX_COUNT||f->value>100u)return 0u;
   const struct track_step *step=&pat->ch[lane][row];
   if(!(key&128u)?!step->effect:!pat->note2_fx[row].effect)return 0u;
   uint8_t duplicates=0u;for(uint8_t y=0u;y<pat->extra_count;y++)duplicates+=(pat->extra[y].id==key);
   if(duplicates>2u)return 0u;
  }''')
patch(base+'track_project_store.c','  const struct track_song_clip *clip=&p->song.clips[r];if(clip->count>TRACK_SONG_FX_EVENTS)return -1;', '''  /* V21: sparse FX2/FX3 after the existing note2 FX pairs. */
  PUT1(pat->extra_count);
  for(uint8_t x=0u;x<pat->extra_count;x++){
   PUT1(pat->extra[x].id);PUT1(pat->extra[x].effect);PUT1(pat->extra[x].value);
  }
  const struct track_song_clip *clip=&p->song.clips[r];if(clip->count>TRACK_SONG_FX_EVENTS)return -1;''')
patch(base+'track_project_store.c','rd16(data+4u)!=19u&&rd16(data+4u)!=TRACK_STORE_VERSION)', 'rd16(data+4u)!=19u&&rd16(data+4u)!=20u&&rd16(data+4u)!=TRACK_STORE_VERSION)')
patch(base+'track_project_store.c','  struct track_song_clip *clip=&p->song.clips[r];GET1(clip->count);', '''  /* Legacy V20 and older projects open without secondary effects. */
  if(rd16(data+4u)>=21u){
   GET1(pat->extra_count);if(pat->extra_count>TRACK_EXTRA_FX_CAP)return -3;
   for(uint8_t x=0u;x<pat->extra_count;x++){
    struct track_extra_fx *f=&pat->extra[x];
    GET1(f->id);GET1(f->effect);GET1(f->value);
    uint8_t key=f->id,lane=(uint8_t)((key&127u)/16u),row=(uint8_t)(key%16u);
    if(lane>=TRACK_CHANNELS||((key&128u)!=0u&&lane!=TRACK_DRUM_CHANNEL)||
       f->effect==0u||f->effect>=TRACK_FX_COUNT||f->value>100u||
       (!(key&128u)?!pat->ch[lane][row].effect:!pat->note2_fx[row].effect))return -3;
    uint8_t duplicates=0u;for(uint8_t j=0u;j<=x;j++)duplicates+=(pat->extra[j].id==key);
    if(duplicates>2u)return -3;
   }
  }
  struct track_song_clip *clip=&p->song.clips[r];GET1(clip->count);''')
# Lane has data if FX only
patch(base+'track_core.c','  if(step->note!=TRACK_NOTE_EMPTY||step->chord!=0u||step->effect!=0u)return 1u;', '  if(step->note!=TRACK_NOTE_EMPTY||step->chord!=0u||step->effect!=0u)return 1u;')
print('stage1 done')
