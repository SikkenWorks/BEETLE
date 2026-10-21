/* SPDX-License-Identifier: GPL-3.0-only
 * V1.3.5: rejected multistep-FX pastes must never partially modify patterns,
 * cursor position, Punch-In events, active FX, or undo history.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_project_store.h"
#include "beetle_pattern_edit.h"
static struct track_song_bank bank;
static struct track_engine eng;
static struct beetle_pattern_editor ed;
static struct beetle_pattern_image pending, whole;
static struct track_step clipboard[TRACK_STEPS];
static struct track_extra_bank saved_fx;
static struct track_pattern saved_pattern;
int main(void){
 track_init(&eng);track_song_init(&bank,&eng);
 memset(&ed,0,sizeof(ed));ed.pending=&pending;ed.whole_clip=&whole;ed.block=clipboard;beetle_edit_reset(&ed);
 eng.transport.edit_channel=0u;
 eng.active.ch[0][2].note=60u;eng.active.ch[0][2].effect=1u;
 const uint8_t key=track_extra_key(0u,2u,0u);
 assert(track_extra_add(&bank,0u,key,0u,2u,35u)==1);
 assert(track_extra_add(&bank,0u,key,1u,5u,72u)==1);
 ed.first=ed.last=2u;ed.action=0u;
 assert(beetle_edit_block_execute(&ed,&bank,&eng)==1);
 beetle_edit_copy_pattern(&ed,&bank,&eng);
 /* Fill 46 more slots, leaving FX1/FX2/FX3 source intact. */
 for(uint8_t i=0u;i<46u;i++){
  const uint8_t pat=(uint8_t)(1u+i/32u);
  const uint8_t lane=(uint8_t)((i%32u)/16u),row=(uint8_t)(i%16u);
  bank.patterns[pat].ch[lane][row].effect=1u;
  assert(track_extra_add(&bank,pat,track_extra_key(lane,row,0u),0u,2u,70u)==1);
 }
 assert(bank.extras.count==48u);
 saved_fx=bank.extras;
 /* Full pattern clipboard must not modify destination on rejection. */
 assert(track_song_switch(&bank,&eng,3u)==0);
 eng.transport.edit_step=12u;eng.active.ch[0][0].note=72u;
 saved_pattern=eng.active;
 uint8_t undo_count=ed.count,undo_cursor=ed.cursor;
 assert(beetle_edit_paste_pattern(&ed,&bank,&eng)==0);
 assert(ed.status==BEETLE_EDIT_FX_POOL_FULL);
 assert(ed.pending_valid==0u);
 assert(eng.transport.edit_step==12u);
 assert(memcmp(&saved_pattern,&eng.active,sizeof(saved_pattern))==0);
 assert(memcmp(&saved_fx,&bank.extras,sizeof(saved_fx))==0);
 assert(ed.count==undo_count&&ed.cursor==undo_cursor);
 /* Block paste is also an atomic rejection. */
 ed.first=4u;ed.last=4u;ed.action=1u;
 assert(beetle_edit_block_execute(&ed,&bank,&eng)==0);
 assert(ed.status==BEETLE_EDIT_FX_POOL_FULL);
 assert(memcmp(&saved_pattern,&eng.active,sizeof(saved_pattern))==0);
 assert(memcmp(&saved_fx,&bank.extras,sizeof(saved_fx))==0);
 assert(ed.count==undo_count&&ed.cursor==undo_cursor);
 /* Free FX slots, then retry: successful copy carries both extra FX. */
 for(int i=47;i>=44;i--)bank.extras.count--;
 assert(bank.extras.count==44u);
 assert(beetle_edit_paste_pattern(&ed,&bank,&eng)==1);
 assert(track_extra_count(&bank,3u,key)==2u);
 assert(eng.active.ch[0][2].note==60u);
 printf("PASS V1.3.5: full-pool block/whole-pattern pastes atomic, correct error, undo stable, retry succeeds\n");
}
