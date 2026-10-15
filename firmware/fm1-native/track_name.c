/* SPDX-License-Identifier: GPL-3.0-only */
#include "track_name.h"
#include "track_core.h"
#include <string.h>
/* First slot is an intentional blank; all other values are editable. */
static const char characters[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.:+/#!?";
uint8_t track_name_charset_size(void){return (uint8_t)(sizeof(characters)-1u);}
char track_name_charset_at(uint8_t index){return index<track_name_charset_size()?characters[index]:' ';}
static uint8_t lookup(char c){for(uint8_t i=0;i<track_name_charset_size();i++)if(characters[i]==c)return i;return 0;}
static uint8_t wrap(int32_t index,int32_t len){int32_t i=index%len;return (uint8_t)(i<0?i+len:i);}
void track_name_begin(struct track_name_editor *ed,const char *existing){
 if(!ed)return;
 memset(ed,0,sizeof(*ed));ed->active=1;
 if(existing){for(uint8_t i=0;i<TRACK_NAME_MAX&&existing[i];i++){
   ed->name[i]=existing[i];ed->touched[i]=1;
   ed->last_index[i]=lookup(existing[i]);ed->remembered_index=ed->last_index[i];
 }}
 ed->glyph_index=ed->touched[0]?ed->last_index[0]:ed->remembered_index;
}
void track_name_move(struct track_name_editor *ed,int32_t detents){
 if(!ed||!ed->active||!detents)return;
 /* K1 moves over cells and does not overwrite the newly entered character. */
 int32_t next=(int32_t)ed->cursor+detents;
 if(next<0)next=0;
 if(next>=(int32_t)TRACK_NAME_MAX)next=TRACK_NAME_MAX-1;
 ed->cursor=(uint8_t)next;
 ed->glyph_index=ed->touched[ed->cursor]?ed->last_index[ed->cursor]:ed->remembered_index;
}
void track_name_change(struct track_name_editor *ed,int32_t detents){
 if(!ed||!ed->active||!detents)return;
 /* Start at the last chosen character, not at 'A' every time. */
 uint8_t next=wrap((int32_t)ed->glyph_index+detents,track_name_charset_size());
 ed->glyph_index=next;ed->remembered_index=next;ed->last_index[ed->cursor]=next;
 ed->name[ed->cursor]=characters[next];ed->touched[ed->cursor]=1;
}
void track_name_finish(struct track_name_editor *ed,uint8_t slot,char output[TRACK_NAME_MAX+1u]){
 if(!ed||!output||slot>=TRACK_PROJECTS)return;
 uint8_t last=0u,has_nonspace=0u;
 for(uint8_t i=0;i<TRACK_NAME_MAX;i++){
  char ch=ed->touched[i]?ed->name[i]:' ';
  output[i]=ch;
  if(ch!=' '){last=i+1u;has_nonspace=1u;}
 }
 if(has_nonspace){output[last]=0;}
 else{
  static const char prefix[]="Project ";
  for(uint8_t i=0;i<8u;i++)output[i]=prefix[i];
  if(slot<9u){output[8]=(char)('1'+slot);output[9]=0;}
  else{output[8]='1';output[9]=(char)('0'+slot-9u);output[10]=0;}
 }
 ed->active=0;
}
