from pathlib import Path
root=Path('/mnt/data/beetle_v134_final/firmware/fm1-native')
def load(fn): return (root/fn).read_text()
def save(fn,s): (root/fn).write_text(s)
# track core: remove pooled methods, keep only id encoder
s=load('track_core.h');start=s.index('/* Sparse secondary FX:');end=s.index('struct track_harmony',start)
s=s[:start]+'''/* Global sparse FX pool is attached to song bank, not the 48 patterns.
 * Pool lives once per project rather than multiplying every undo snapshot. */
struct track_pattern {struct track_step ch[TRACK_CHANNELS][TRACK_STEPS];uint8_t length;
 struct track_note2_fx note2_fx[TRACK_STEPS];};
static inline uint8_t track_extra_key(uint8_t lane,uint8_t row,uint8_t note2){
 return (uint8_t)(lane*TRACK_STEPS+row+(note2?128u:0u));
}
'''+s[end:]
s=s.replace('#define TRACK_EXTRA_FX_CAP 16u /* up to sixteen secondary FX per pattern, two per step */','#define TRACK_EXTRA_FX_CAP 48u /* project-wide sparse FX2/FX3 capacity */')
save('track_core.h',s)
s=load('track_core.c');s=s.replace(' track_extra_clear(&e->active,track_extra_key(channel,step,0u));\n if(channel==TRACK_DRUM_CHANNEL)track_extra_clear(&e->active,track_extra_key(channel,step,1u));\n',' ');save('track_core.c',s)
# FX bank in track_song.h, with functions
s=load('track_song.h');needle=' uint8_t edit_row,edit_column,composite_active;\n};'
new=''' uint8_t edit_row,edit_column,composite_active;
 /* Shared project-wide sparse FX pool: each record belongs to a pattern. */
 struct track_extra_bank {
  uint8_t count;
  struct track_extra_fx {uint8_t pattern,id,effect,value;} data[TRACK_EXTRA_FX_CAP];
 } extras;
};
static inline uint8_t track_extra_count(const struct track_song_bank *b,uint8_t pattern,uint8_t key){
 uint8_t n=0u;if(!b)return 0u;
 for(uint8_t i=0u;i<b->extras.count && i<TRACK_EXTRA_FX_CAP;i++)
  if(b->extras.data[i].pattern==pattern&&b->extras.data[i].id==key)n++;
 return n;
}
static inline struct track_extra_fx *track_extra_get(struct track_song_bank *b,uint8_t pattern,uint8_t key,uint8_t slot){
 if(!b||slot>=2u)return 0;
 for(uint8_t i=0u;i<b->extras.count&&i<TRACK_EXTRA_FX_CAP;i++)
  if(b->extras.data[i].pattern==pattern&&b->extras.data[i].id==key){
   if(!slot)return &b->extras.data[i];slot--;
  }
 return 0;
}
static inline const struct track_extra_fx *track_extra_find(const struct track_song_bank *b,uint8_t pattern,uint8_t key,uint8_t slot){
 return track_extra_get((struct track_song_bank*)b,pattern,key,slot);
}
static inline void track_extra_remove(struct track_song_bank *b,uint8_t pattern,uint8_t key,uint8_t slot){
 if(!b||slot>=2u)return;
 for(uint8_t i=0u,matched=0u;i<b->extras.count;i++)
  if(b->extras.data[i].pattern==pattern&&b->extras.data[i].id==key){
   if(matched++==slot){for(uint8_t j=i;j+1u<b->extras.count;j++)b->extras.data[j]=b->extras.data[j+1u];
    b->extras.count--;return;}
  }
}
static inline void track_extra_clear(struct track_song_bank *b,uint8_t pattern,uint8_t key){
 while(track_extra_count(b,pattern,key))track_extra_remove(b,pattern,key,0u);
}
static inline int track_extra_add(struct track_song_bank *b,uint8_t pattern,uint8_t key,uint8_t slot,uint8_t fx,uint8_t value){
 if(!b||pattern>=TRACK_PATTERNS||slot>=2u||fx==0u||fx>=TRACK_FX_COUNT||value>100u)return -2;
 if(track_extra_get(b,pattern,key,slot))return 0;
 if(track_extra_count(b,pattern,key)!=slot)return -2;
 if(b->extras.count>=TRACK_EXTRA_FX_CAP)return -1;
 b->extras.data[b->extras.count++]=(struct track_extra_fx){pattern,key,fx,value};return 1;
}
/* Remove stale extra FX after editing/clearing the primary FX field. */
static inline void track_extra_prune(struct track_song_bank *b,uint8_t pattern,const struct track_pattern *p){
 if(!b||!p)return;
 for(uint8_t i=0u;i<b->extras.count;){
  struct track_extra_fx *f=&b->extras.data[i];
  if(f->pattern!=pattern){i++;continue;}
  uint8_t lane=(uint8_t)((f->id&127u)/TRACK_STEPS),row=(uint8_t)((f->id&127u)%TRACK_STEPS);
  uint8_t primary=lane<TRACK_CHANNELS?((f->id&128u)?p->note2_fx[row].effect:p->ch[lane][row].effect):0u;
  if(!primary){for(uint8_t j=i;j+1u<b->extras.count;j++)b->extras.data[j]=b->extras.data[j+1u];b->extras.count--;}
  else i++;
 }
}'''
assert needle in s;s=s.replace(needle,new);save('track_song.h',s)
# song copy and wipe: remove composite extra_count
s=load('track_song.c');s=s.replace(' out->extra_count=0u;\n','');s=s.replace(' b->patterns[e->pattern_slot]=e->active;',' b->patterns[e->pattern_slot]=e->active;\n track_extra_prune(b,e->pattern_slot,&e->active);');save('track_song.c',s)
# persistence adapt to global per pattern
s=load('track_project_store.c')
start=s.index('  if(pat->length<1u || pat->length>TRACK_STEPS||pat->extra_count>TRACK_EXTRA_FX_CAP)return 0u;')
end=s.index('  for(uint8_t s=0u;s<TRACK_STEPS;s++)',start)
s=s[:start]+'''  if(pat->length<1u || pat->length>TRACK_STEPS || p->song.extras.count>TRACK_EXTRA_FX_CAP)return 0u;
  for(uint8_t x=0u;x<p->song.extras.count;x++){
   const struct track_extra_fx *f=&p->song.extras.data[x];
   if(f->pattern!=k)continue;
   uint8_t key=f->id,lane=(uint8_t)((key&127u)/TRACK_STEPS),row=(uint8_t)((key&127u)%TRACK_STEPS);
   if(lane>=TRACK_CHANNELS||((key&128u)!=0u&&lane!=TRACK_DRUM_CHANNEL)||
      f->effect==0u||f->effect>=TRACK_FX_COUNT||f->value>100u)return 0u;
   if(!(key&128u)?!pat->ch[lane][row].effect:!pat->note2_fx[row].effect)return 0u;
   if(track_extra_count(&p->song,k,key)>2u)return 0u;
  }
'''+s[end:]
start=s.index('  /* V21: sparse FX2/FX3 after the existing note2 FX pairs. */')
end=s.index('  const struct track_song_clip *clip',start)
s=s[:start]+'''  /* V21: per-pattern sparse slice of the global FX pool. */
  uint8_t extra_count=0u;
  for(uint8_t x=0u;x<p->song.extras.count;x++)
   if(p->song.extras.data[x].pattern==r)extra_count++;
  PUT1(extra_count);
  for(uint8_t x=0u;x<p->song.extras.count;x++){
   const struct track_extra_fx *f=&p->song.extras.data[x];if(f->pattern!=r)continue;
   PUT1(f->id);PUT1(f->effect);PUT1(f->value);
  }
'''+s[end:]
start=s.index('  /* Legacy V20 and older projects open without secondary effects. */')
end=s.index('  struct track_song_clip *clip',start)
s=s[:start]+'''  /* Decode sparse per-pattern FX entries into the shared bank. */
  if(rd16(data+4u)>=21u){
   uint8_t count;GET1(count);
   if(count>TRACK_EXTRA_FX_CAP || p->song.extras.count+count>TRACK_EXTRA_FX_CAP)return -3;
   for(uint8_t x=0u;x<count;x++){
    uint8_t key,fx,value;GET1(key);GET1(fx);GET1(value);
    uint8_t lane=(uint8_t)((key&127u)/16u),row=(uint8_t)((key&127u)%16u);
    if(lane>=TRACK_CHANNELS||((key&128u)!=0u&&lane!=TRACK_DRUM_CHANNEL)||
       !fx||fx>=TRACK_FX_COUNT||value>100u||
       (!(key&128u)?!pat->ch[lane][row].effect:!pat->note2_fx[row].effect) ||
       track_extra_count(&p->song,r,key)>=2u)return -3;
    p->song.extras.data[p->song.extras.count++]=(struct track_extra_fx){r,key,fx,value};
   }
  }
'''+s[end:]
save('track_project_store.c',s)
# runtime: apply global signatures; physical editing works only to fixed source pattern
s=load('track_runtime.h')
s=s.replace('track_extra_clear(&track_state.active,track_extra_key(track_state.transport.edit_channel,track_state.transport.edit_step,0u))','track_extra_clear(&track_song,track_state.pattern_slot,track_extra_key(track_state.transport.edit_channel,track_state.transport.edit_step,0u))')
s=s.replace(''' const struct track_pattern *p=&track_state.active;
 if(track_song.enabled&&track_song.composite_active&&track_song.position<track_song.count){
  uint8_t src=track_song.chain[track_song.position].lane[lane];
  if(src>=TRACK_PATTERNS)return 0;
  p=&track_song.patterns[src];row=(uint8_t)(row%(p->length?p->length:16u));
 }
 return track_extra_find(p,track_extra_key(lane,row,note2),slot);''',''' uint8_t pat=track_state.pattern_slot;
 if(track_song.enabled&&track_song.composite_active&&track_song.position<track_song.count){
  uint8_t src=track_song.chain[track_song.position].lane[lane];
  if(src>=TRACK_PATTERNS)return 0;
  pat=src;
  const struct track_pattern *p=&track_song.patterns[src];
  row=(uint8_t)(row%(p->length?p->length:16u));
 }
 return track_extra_find(&track_song,pat,track_extra_key(lane,row,note2),slot);''')
s=s.replace('track_extra_count(&track_state.active,track_extra_key(track_state.transport.edit_channel,row,0u))','track_extra_count(&track_song,track_state.pattern_slot,track_extra_key(track_state.transport.edit_channel,row,0u))')
s=s.replace('track_extra_find(&track_state.active,key,','track_extra_find(&track_song,track_state.pattern_slot,key,')
s=s.replace('track_extra_remove(&track_state.active,key,','track_extra_remove(&track_song,track_state.pattern_slot,key,')
s=s.replace('track_extra_clear(&track_state.active,key)','track_extra_clear(&track_song,track_state.pattern_slot,key)')
s=s.replace('track_extra_get(&track_state.active,key,','track_extra_get(&track_song,track_state.pattern_slot,key,')
s=s.replace('track_extra_add(&track_state.active,key,','track_extra_add(&track_song,track_state.pattern_slot,key,')
s=s.replace('track_state.active.extra_count','track_song.extras.count')
save('track_runtime.h',s)
print('global patch done')
