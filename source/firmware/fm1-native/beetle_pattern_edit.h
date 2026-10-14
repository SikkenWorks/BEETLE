/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE V0.56: bounded, RAM-only pattern editing. No ISR allocations or flash IO.
 * Snapshot history is per EDIT ACTION and includes the recorded Punch-In lane.
 */
#ifndef BEETLE_PATTERN_EDIT_H
#define BEETLE_PATTERN_EDIT_H
#include "track_song.h"
#define BEETLE_UNDO_CAP 6u
#define BEETLE_BLOCK_ACTIONS 7u
struct beetle_pattern_image {
 struct track_pattern pattern;
 struct track_song_clip punch;
};
struct beetle_history_entry {
 struct beetle_pattern_image state;
 uint8_t pattern_index;
};
struct beetle_pattern_editor {
 struct beetle_history_entry history[BEETLE_UNDO_CAP];
 struct beetle_pattern_image *pending,*whole_clip;
 struct track_step *block;
 uint8_t cursor,count,pending_valid,pending_index,whole_valid,block_valid,block_count;
 uint8_t first,last,action;
 int8_t amount;
 uint8_t status;
};
static inline void beetle_edit_reset(struct beetle_pattern_editor *ed){
 if(!ed)return;
 ed->cursor=ed->count=ed->pending_valid=ed->whole_valid=ed->block_valid=0u;
 ed->block_count=ed->first=ed->last=ed->action=ed->status=0u;ed->amount=1;
}
static inline uint8_t beetle_image_different(const struct beetle_pattern_image *a,const struct beetle_pattern_image *b){
 const uint8_t *p=(const uint8_t*)a,*q=(const uint8_t*)b;
 for(uint32_t i=0u;i<sizeof(*a);i++)if(p[i]!=q[i])return 1u;
 return 0u;
}
static inline void beetle_edit_capture(struct beetle_pattern_image *img,const struct track_song_bank *bank,const struct track_engine *eng){
 img->pattern=eng->active;img->punch=bank->clips[eng->pattern_slot];
}
static inline void beetle_edit_begin(struct beetle_pattern_editor *ed,const struct track_song_bank *bank,const struct track_engine *eng){
 if(!ed||!bank||!eng||ed->pending_valid)return;
 beetle_edit_capture(ed->pending,bank,eng);
 ed->pending_index=eng->pattern_slot;ed->pending_valid=1u;
}
static inline int beetle_edit_commit(struct beetle_pattern_editor *ed,const struct track_song_bank *bank,const struct track_engine *eng){
 if(!ed||!ed->pending_valid)return 0;
 if(ed->pending_index!=eng->pattern_slot){ed->pending_valid=0u;return 0;}
 ed->pending_valid=0u;
 uint8_t changed=0u;
 const uint8_t *before=(const uint8_t*)&ed->pending->pattern,*after=(const uint8_t*)&eng->active;
 for(uint32_t i=0u;i<sizeof(eng->active);i++)if(before[i]!=after[i]){changed=1u;break;}
 if(!changed){
  before=(const uint8_t*)&ed->pending->punch;
  after=(const uint8_t*)&bank->clips[eng->pattern_slot];
  for(uint32_t i=0u;i<sizeof(ed->pending->punch);i++)if(before[i]!=after[i]){changed=1u;break;}
 }
 if(!changed)return 0;
 if(ed->cursor<ed->count)ed->count=ed->cursor; /* invalidate redo branch */
 if(ed->count==BEETLE_UNDO_CAP){
  for(uint8_t i=1u;i<BEETLE_UNDO_CAP;i++)ed->history[i-1u]=ed->history[i];
  ed->count--;if(ed->cursor)ed->cursor--;
 }
 struct beetle_history_entry *entry=&ed->history[ed->count++];
 entry->state=*ed->pending;entry->pattern_index=eng->pattern_slot;
 ed->cursor=ed->count;return 1;
}
static inline int beetle_edit_apply_image(struct track_song_bank *bank,struct track_engine *eng,uint8_t pat,const struct beetle_pattern_image *img){
 if(!bank||!eng||!img||pat>=TRACK_PATTERNS)return 0;
 if(pat!=eng->pattern_slot)(void)track_song_switch(bank,eng,pat);
 eng->active=img->pattern;bank->patterns[pat]=img->pattern;
 if(eng->active.length>=1u&&eng->active.length<=TRACK_STEPS && eng->transport.edit_step>=eng->active.length)
  eng->transport.edit_step=(uint8_t)(eng->active.length-1u);
 bank->clips[pat]=img->punch;eng->dirty=1u;return 1;
}
/* Undo/redo use the same snapshot slot. On every move, swap its pattern
 * image against the pattern currently in RAM. Only one history image/action. */
static inline int beetle_edit_swap(struct beetle_pattern_editor *ed,struct track_song_bank *bank,struct track_engine *eng,uint8_t index){
 struct beetle_history_entry *entry=&ed->history[index];
 if(entry->pattern_index!=eng->pattern_slot)(void)track_song_switch(bank,eng,entry->pattern_index);
 beetle_edit_capture(ed->pending,bank,eng);
 (void)beetle_edit_apply_image(bank,eng,entry->pattern_index,&entry->state);
 entry->state=*ed->pending;
 return 1;
}
static inline int beetle_edit_undo(struct beetle_pattern_editor *ed,struct track_song_bank *bank,struct track_engine *eng){
 if(!ed||!ed->cursor||ed->pending_valid)return 0;
 return beetle_edit_swap(ed,bank,eng,--ed->cursor);
}
static inline int beetle_edit_redo(struct beetle_pattern_editor *ed,struct track_song_bank *bank,struct track_engine *eng){
 if(!ed||ed->cursor>=ed->count||ed->pending_valid)return 0;
 return beetle_edit_swap(ed,bank,eng,ed->cursor++);
}
static inline void beetle_edit_copy_pattern(struct beetle_pattern_editor *ed,const struct track_song_bank *bank,const struct track_engine *eng){
 if(!ed)return;
 beetle_edit_capture(ed->whole_clip,bank,eng);ed->whole_valid=1u;
}
static inline int beetle_edit_paste_pattern(struct beetle_pattern_editor *ed,struct track_song_bank *bank,struct track_engine *eng){
 if(!ed||!ed->whole_valid)return 0;
 beetle_edit_begin(ed,bank,eng);
 eng->active=ed->whole_clip->pattern;
 if(eng->active.length>=1u&&eng->active.length<=TRACK_STEPS && eng->transport.edit_step>=eng->active.length)
  eng->transport.edit_step=(uint8_t)(eng->active.length-1u);
 bank->clips[eng->pattern_slot]=ed->whole_clip->punch;
 if(beetle_edit_commit(ed,bank,eng)){eng->dirty=1u;return 1;}
 return 0;
}
static inline uint8_t beetle_block_bound_length(int32_t v,uint8_t length){
 if(length<1u||length>TRACK_STEPS)length=TRACK_STEPS;
 return (uint8_t)(v<0?0:v>=(int32_t)length?length-1u:v);
}
static inline uint8_t beetle_block_bound(int32_t v){return beetle_block_bound_length(v,TRACK_STEPS);}
static inline void beetle_edit_block_copy(struct beetle_pattern_editor *ed,const struct track_engine *eng){
 if(!ed||!eng)return;
 uint8_t a=ed->first<ed->last?ed->first:ed->last;
 uint8_t b=ed->first>ed->last?ed->first:ed->last;
 for(uint8_t s=a;s<=b;s++)ed->block[s-a]=eng->active.ch[eng->transport.edit_channel][s];
 ed->block_count=(uint8_t)(b-a+1u);ed->block_valid=1u;
}
static inline void beetle_edit_blank(struct track_step *st){
 st->note=TRACK_NOTE_EMPTY;st->chord=0u;st->effect=0u;st->value=50u;st->dynamics=0u;
}
/* Operation 0 copy, 1 paste at FIRST, 2 clear, 3 transpose, 4 shift with
 * zero-blank vacated cells, 5 undo, 6 redo. Selection is limited to one lane.
 * Whole-pattern operations are independent and include all 6 lanes + Punch. */
static inline int beetle_edit_block_execute(struct beetle_pattern_editor *ed,struct track_song_bank *bank,struct track_engine *eng){
 if(!ed||!bank||!eng||eng->transport.edit_channel>=TRACK_CHANNELS)return 0;
 /* Restrict destructive block edits to active rows. Steps beyond the
  * pattern end must survive a later expansion back to 16. */
 const uint8_t len=(eng->active.length>=1u&&eng->active.length<=TRACK_STEPS)?eng->active.length:TRACK_STEPS;
 ed->first=beetle_block_bound_length(ed->first,len);
 ed->last=beetle_block_bound_length(ed->last,len);
 const uint8_t a=ed->first<ed->last?ed->first:ed->last;
 const uint8_t b=ed->first>ed->last?ed->first:ed->last;
 const uint8_t ch=eng->transport.edit_channel;
 if(ed->action==0u){beetle_edit_block_copy(ed,eng);ed->status=1u;return 1;}
 if(ed->action==5u){ed->status=(uint8_t)(beetle_edit_undo(ed,bank,eng)?2u:4u);return ed->status==2u;}
 if(ed->action==6u){ed->status=(uint8_t)(beetle_edit_redo(ed,bank,eng)?3u:4u);return ed->status==3u;}
 if(ed->action==1u&&!ed->block_valid){ed->status=4u;return 0;}
 beetle_edit_begin(ed,bank,eng);
 if(ed->action==1u){
  for(uint8_t i=0u;i<ed->block_count&&ed->first+i<len;i++)eng->active.ch[ch][ed->first+i]=ed->block[i];
 }else if(ed->action==2u){
  for(uint8_t i=a;i<=b;i++)beetle_edit_blank(&eng->active.ch[ch][i]);
 }else if(ed->action==3u && ch!=(TRACK_CHANNELS-1u)){
  for(uint8_t i=a;i<=b;i++){
   struct track_step *st=&eng->active.ch[ch][i];
   if(st->note<=127u){int32_t pitch=(int32_t)st->note+ed->amount;
    st->note=(uint8_t)(pitch<0?0:pitch>127?127:pitch);}
  }
 }else if(ed->action==4u){
  if(ed->amount>0){for(uint8_t i=b;i>a;i--)eng->active.ch[ch][i]=eng->active.ch[ch][i-1u];beetle_edit_blank(&eng->active.ch[ch][a]);}
  else if(ed->amount<0){for(uint8_t i=a;i<b;i++)eng->active.ch[ch][i]=eng->active.ch[ch][i+1u];beetle_edit_blank(&eng->active.ch[ch][b]);}
 }
 if(beetle_edit_commit(ed,bank,eng)){eng->dirty=1u;ed->status=2u;return 1;}
 ed->status=4u;return 0;
}
#endif
