/* GPL-3.0-only: V1.3.4 multi-FX block copy/paste/shift regression. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_project_store.h"
#include "beetle_pattern_edit.h"
static struct track_song_bank bank;
static struct track_engine engine;
static struct beetle_pattern_editor ed;
static struct beetle_pattern_image pending,whole;
static struct track_step block[TRACK_STEPS];
int main(void){
 track_init(&engine);track_song_init(&bank,&engine);
 memset(&ed,0,sizeof(ed));ed.pending=&pending;ed.whole_clip=&whole;ed.block=block;beetle_edit_reset(&ed);
 engine.transport.edit_channel=0;
 engine.active.ch[0][2].note=60;
 engine.active.ch[0][2].effect=1;
 uint8_t key=track_extra_key(0,2,0);
 assert(track_extra_add(&bank,0,key,0,2,30)==1);
 assert(track_extra_add(&bank,0,key,1,4,55)==1);
 ed.first=2;ed.last=2;ed.action=0;
 assert(beetle_edit_block_execute(&ed,&bank,&engine));
 assert(ed.block_fx_count==2);
 ed.first=4;ed.last=4;ed.action=1;
 assert(beetle_edit_block_execute(&ed,&bank,&engine));
 key=track_extra_key(0,4,0);
 assert(track_extra_count(&bank,0,key)==2);
 assert(track_extra_find(&bank,0,key,0)->effect==2);
 assert(engine.active.ch[0][4].note==60);
 ed.first=4;ed.last=5;ed.action=4;ed.amount=1;
 assert(beetle_edit_block_execute(&ed,&bank,&engine));
 assert(track_extra_count(&bank,0,track_extra_key(0,4,0))==0);
 assert(track_extra_count(&bank,0,track_extra_key(0,5,0))==2);
 assert(engine.active.ch[0][5].note==60);
 assert(beetle_edit_undo(&ed,&bank,&engine));
 assert(engine.active.ch[0][4].note==60);
 assert(track_extra_count(&bank,0,track_extra_key(0,4,0))==2);
 printf("PASS V1.3.4: block copy/paste/shift preserves multi-FX; undo restores FX state\n");
}
