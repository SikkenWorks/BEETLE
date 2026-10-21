/* BEETLE V0.68 independent mute group, snapshot interoperability and project V14 migration. */
#include "track_project_store.h"
#include "track_master_fx.h"
#include "beetle_mute_groups.h"
#include "beetle_performance_snapshots.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct track_store_project p, q;
static unsigned char raw[TRACK_STORE_MAX];
static void wr32(unsigned char *dst,unsigned v){for(unsigned i=0;i<4;i++)dst[i]=(unsigned char)(v>>(i*8));}
static void hash(unsigned n){wr32(raw+8,n);wr32(raw+12,track_store_crc32(raw+24,n-24));wr32(raw+16,track_store_crc32(raw,16));}
int main(void){
 struct track_engine state;track_init(&state);track_song_init(&p.song,&state);
 track_mixer_init(&p.mixer);track_master_default(&p.master);
 for(unsigned i=0;i<4;i++){p.instruments[i].volume=60;track_va_patch_default(&p.instruments[i].va);}
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&p.drum[i]);
 p.slot=1;p.bpm=120;
 beetle_snapshots_init(&p.snapshots);beetle_groups_init(&p.groups);
 struct beetle_mute_queue queue={0};
 assert(sizeof(p.groups)==4);
 assert(!beetle_group_toggle(&p.groups,0,&queue,&p.mixer,1));
 assert(beetle_group_member_toggle(&p.groups,0,0));
 assert(beetle_group_member_toggle(&p.groups,0,2));
 assert(beetle_group_member_toggle(&p.groups,0,5));
 assert(p.groups.masks[0]==37);
 assert(!beetle_group_member_toggle(&p.groups,4,0));
 p.mixer.ch[4].mute=0;
 p.mixer.quantize_mutes=1;
 /* A queued change outside the group cannot be erased by group toggles. */
 assert(beetle_mute_arm(&queue,&p.mixer,4,1));
 assert(beetle_group_toggle(&p.groups,0,&queue,&p.mixer,1));
 assert(queue.pending==(37|16) && queue.values==(37|16));
 assert(!p.mixer.ch[0].mute && !p.mixer.ch[2].mute && !p.mixer.ch[5].mute);
 /* Queued target is considered when toggling a group again. */
 assert(beetle_group_toggle(&p.groups,0,&queue,&p.mixer,1));
 assert(queue.pending==16 && queue.values==16);
 assert(beetle_group_toggle(&p.groups,0,&queue,&p.mixer,1));
 assert(queue.pending==(37|16));
 assert(beetle_mute_commit(&queue,&p.mixer));
 assert(p.mixer.ch[0].mute && p.mixer.ch[2].mute && p.mixer.ch[5].mute && p.mixer.ch[4].mute);
 p.mixer.quantize_mutes=0;
 assert(beetle_group_toggle(&p.groups,0,&queue,&p.mixer,1));
 assert(!p.mixer.ch[0].mute && !p.mixer.ch[2].mute && !p.mixer.ch[5].mute && p.mixer.ch[4].mute);
 /* Snapshots recall whole state; groups only member lanes. */
 assert(beetle_snapshot_store(&p.snapshots,0,&p.mixer));
 assert(beetle_group_toggle(&p.groups,0,&queue,&p.mixer,0));
 assert(beetle_snapshot_recall(&p.snapshots,0,&queue,&p.mixer,0));
 assert(beetle_snapshot_mask(&p.mixer)==16);
 p.groups.masks[3]=63;
 unsigned n;assert(track_store_encode(&p,raw,sizeof raw,&n)==0);
 assert(raw[4]==TRACK_STORE_VERSION && raw[5]==0);
 assert(track_store_decode(&q,raw,n)==0);
 assert(q.groups.masks[0]==37 && q.groups.masks[3]==63);
 assert(q.snapshots.valid==1 && q.snapshots.masks[0]==16);
 /* Real V14-shaped payload retains snapshots, migrates groups to empty. */
 unsigned older=n-5;raw[4]=14;raw[5]=0;hash(older);
 assert(track_store_decode(&q,raw,older)==0);
 assert(q.snapshots.valid==1 && q.snapshots.masks[0]==16);
 for(unsigned i=0;i<4;i++)assert(q.groups.masks[i]==0);
 /* V13 additionally has no snapshot bytes. */
 older-=5;raw[4]=13;hash(older);
 assert(track_store_decode(&q,raw,older)==0);
 assert(q.snapshots.valid==0 && q.groups.masks[0]==0);
 /* Reject malformed group membership even when CRC is forged. */
 assert(track_store_encode(&p,raw,sizeof raw,&n)==0);
 raw[n-2]=0x40;hash(n);
 assert(track_store_decode(&q,raw,n)==-3);
 p.groups.masks[1]=0x40;
 assert(track_store_encode(&p,raw,sizeof raw,&n)==-1);
 puts("PASS V0.68: group membership, queued/direct toggle, nonmember isolation, snapshot interoperability, V15 codec, V14/V13 migration, CRC checks");
 return 0;
}
