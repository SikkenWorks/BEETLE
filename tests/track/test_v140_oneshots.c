/* SPDX-License-Identifier: GPL-3.0-only -- user one-shot A/B journal codec & DMA-safe PCM regression */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_core.h"
#include "track_drum.h"
#include "track_project_store.h"
#include "track_master_fx.h"
static struct track_store_project a,b;
static struct track_engine e;
static uint8_t first1[BEETLE_ONESHOT_FIRST_BYTES],second1[BEETLE_ONESHOT_SECOND_BYTES];
static uint8_t first2[BEETLE_ONESHOT_FIRST_BYTES],second2[BEETLE_ONESHOT_SECOND_BYTES];
static uint8_t encoded[TRACK_STORE_V4_MAX];
static int16_t out[2048];
static uint64_t energy(const int16_t *out,unsigned count){uint64_t result=0;for(unsigned i=0;i<count;i++)result+=(int64_t)out[i]*out[i];return result;}
int main(void){
 track_init(&e);track_song_init(&a.song,&e);track_mixer_init(&a.mixer);track_master_default(&a.master);
 a.bpm=120;a.slot=0;
 for(unsigned i=0;i<8;i++){a.instruments[i].volume=70;track_va_patch_default(&a.instruments[i].va);}
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_component_default(&a.drum[i],(uint8_t)i);
 for(unsigned i=0;i<5;i++)a.punch_param[i]=50;
 a.oneshots.first=first1;a.oneshots.second=second1;
 b.oneshots.first=first2;b.oneshots.second=second2;
 uint32_t n=0;assert(track_store_encode(&a,encoded,sizeof encoded,&n)==0);
 assert(encoded[4]==22u);assert(track_store_decode(&b,encoded,n)==0);
 assert(track_note_scroll(13,1,1)==14);assert(track_note_scroll(26,1,1)==TRACK_NOTE_EMPTY);
 assert(track_enter_note(&e,14)==0);assert(e.active.ch[0][0].note==14);
 // Editable drum note follows physical keys, MIDI note2 stays old 4-bit mapping.
 e.transport.edit_channel=TRACK_DRUM_CHANNEL;
 assert(track_enter_note(&e,14)==0);
 assert(track_enter_note(&e,26)==0);
 assert(track_enter_drum_note2(&e,26)!=0);
 a.song.patterns[0].ch[TRACK_DRUM_CHANNEL][0].note=14;
 a.song.patterns[0].ch[TRACK_DRUM_CHANNEL][1].note=26;
 a.oneshots.length[0]=1150;
 a.oneshots.length[12]=1500;
 memcpy(a.oneshots.name[0],"SNAP",4);memcpy(a.oneshots.name[12],"HIT",3);
 for(unsigned i=0;i<2650;i++)beetle_oneshot_set(&a.oneshots,i,(uint8_t)(i%224u+16u));
 assert(track_store_encode(&a,encoded,sizeof encoded,&n)==0);assert(encoded[4]==23u);
 assert(track_store_decode(&b,encoded,n)==0);
 assert(b.oneshots.length[0]==1150&&b.oneshots.length[12]==1500);
 assert(b.song.patterns[0].ch[TRACK_DRUM_CHANNEL][0].note==14);
 assert(b.song.patterns[0].ch[TRACK_DRUM_CHANNEL][1].note==26);
 for(unsigned i=0;i<2650;i++)assert(beetle_oneshot_get(&b.oneshots,i)==(uint8_t)(i%224u+16u));
 struct track_drum_engine drum; track_drum_init(&drum);track_drum_bind_oneshots(&drum,&b.oneshots);
 assert(track_drum_trigger(&drum,14)==1);assert(track_drum_trigger(&drum,26)==1);
 assert(track_drum_trigger(&drum,0)==1);assert(track_drum_trigger(&drum,27)==0);
 assert(track_drum_trigger_pair(&drum,14,1)==2);
 for(unsigned i=0;i<6;i++)assert(track_drum_trigger(&drum,(uint8_t)(i%2?14:26))==1);
 memset(out,0,sizeof out);track_drum_render(&drum,&b.mixer,out,1024u);
 assert(energy(out,2048u)>1000000u);
 // Corrupted/wrong-length bank must fail closed despite a recomputed CRC.
 uint32_t bank_len_offset=n-6u;
 uint16_t intact_length=b.oneshots.length[0];uint8_t intact_pcm=beetle_oneshot_get(&b.oneshots,0u);
 encoded[bank_len_offset]^=1;
 uint32_t crc=track_store_crc32(encoded+24u,n-24u);
 encoded[12]=(uint8_t)crc;encoded[13]=(uint8_t)(crc>>8u);encoded[14]=(uint8_t)(crc>>16u);encoded[15]=(uint8_t)(crc>>24u);
 crc=track_store_crc32(encoded,16u);
 encoded[16]=(uint8_t)crc;encoded[17]=(uint8_t)(crc>>8u);encoded[18]=(uint8_t)(crc>>16u);encoded[19]=(uint8_t)(crc>>24u);
 assert(track_store_decode(&b,encoded,n)!=0);
 assert(b.oneshots.length[0]==intact_length && beetle_oneshot_get(&b.oneshots,0u)==intact_pcm);
 // 13-slot bank hard cap. Do not spill into project or boot storage.
 a.oneshots.length[0]=6144;a.oneshots.length[12]=1;
 assert(track_store_encode(&a,encoded,sizeof encoded,&n)!=0);
 puts("PASS V1.4.0: legacy V22, V23 CRC roundtrip, keyboard notes, 13 sample slots, native PCM, bounded storage");
}
