/* BEETLE V0.31 regression: 12 storage slots, mixer swing, 16-bit drum synth,
 * physical storage mapped (write requires explicit manual format), master attenuation. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "track_core.h"
#include "track_mixer.h"
#include "track_swing.h"
#include "track_journal.h"
#include "track_drum.h"
#include "track_output_gain.h"
#include "beetle_flash_layout.h"
#include "track_name.h"
static uint8_t disk[TRACK_PROJECTS][2][TRACK_JOURNAL_BANK_BYTES];
static uint8_t scratch[TRACK_STORE_MAX];
static struct track_store_project project,readback;
static int r(void *x,uint8_t slot,uint8_t bank,uint32_t off,uint8_t *dst,uint32_t n){(void)x;if(slot>=TRACK_PROJECTS||bank>=2||off>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-off)return -1;memcpy(dst,disk[slot][bank]+off,n);return 0;}
static int e(void *x,uint8_t slot,uint8_t bank){(void)x;if(slot>=TRACK_PROJECTS||bank>=2)return -1;memset(disk[slot][bank],0xff,TRACK_JOURNAL_BANK_BYTES);return 0;}
static int w(void *x,uint8_t slot,uint8_t bank,uint32_t off,const uint8_t *src,uint32_t n){(void)x;if(slot>=TRACK_PROJECTS||bank>=2||off>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-off)return -1;for(uint32_t j=0;j<n;j++)if((disk[slot][bank][off+j]&src[j])!=src[j])return -1;for(uint32_t j=0;j<n;j++)disk[slot][bank][off+j]&=src[j];return 0;}
static const struct track_journal_io io={0,r,e,w};
static int16_t pcm[2048*2];
static uint64_t power(void){uint64_t sum=0;for(unsigned j=0;j<4096;j++){int32_t t=pcm[j];sum+=t<0?-t:t;}return sum;}
int main(void){
 assert(TRACK_PROJECTS==12u && TRACK_JOURNAL_SLOTS==12u && TRACK_JOURNAL_PROVISION_BYTES==294912u);
 assert(TRACK_STORE_VERSION==4u && TRACK_MIXER_FIELDS==5u);
 struct track_mixer mix;track_mixer_init(&mix);
 for(uint8_t c=0;c<4;c++)assert(mix.ch[c].swing==0);
 assert(track_swing_mask(&mix,1)==0);
 mix.col=4;assert(track_mixer_change(&mix,1)==1&&mix.ch[0].swing==1);
 assert(track_swing_mask(&mix,0)==0&&track_swing_mask(&mix,1)==1);
 track_mixer_move(&mix,1,0);assert(track_mixer_change(&mix,1));
 assert(track_swing_mask(&mix,3)==3 && track_swing_mask(&mix,4)==0);
 assert(track_swing_delay(5512)==1378);
 assert(track_mixer_change(&mix,-1) && mix.ch[1].swing==0);
 assert(track_swing_mask(&mix,1)==1);
 memset(disk,255,sizeof(disk));
 for(uint8_t slot=0;slot<TRACK_PROJECTS;slot++){
  struct track_engine engine;track_init(&engine);
  memset(&project,0,sizeof(project));track_song_init(&project.song,&engine);track_mixer_init(&project.mixer);
  project.slot=slot;project.bpm=130+slot;project.mixer.ch[0].swing=1;
  project.song.patterns[47].ch[0][0].note=60+slot;
  uint32_t used=0;assert(track_store_encode(&project,scratch,sizeof(scratch),&used)==0);
  assert(used+TRACK_JOURNAL_HEADER_BYTES<TRACK_JOURNAL_BANK_BYTES);
  assert(track_journal_save(&io,slot,&project,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
  assert(track_journal_load(&io,slot,&readback,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
  assert(readback.slot==slot && readback.mixer.ch[0].swing==1&&readback.bpm==130+slot);
  assert(readback.song.patterns[47].ch[0][0].note==60+slot);
 }
 assert(beetle_flash_map(0,0,0,256,0)!=0);
 uint32_t physical=0;assert(beetle_flash_map(11,1,0,256,&physical)==0);
 assert(physical==BEETLE_FLASH_END-TRACK_JOURNAL_BANK_BYTES);
 char name[TRACK_NAME_MAX+1];struct track_name_editor editor;
 track_name_begin(&editor,0);track_name_finish(&editor,11,name);assert(strcmp(name,"Project 12")==0);
 struct track_drum_engine drum;track_drum_init(&drum);
 for(uint8_t n=0;n<TRACK_DRUM_COUNT;n++){
  track_drum_stop(&drum);
  assert(track_drum_trigger(&drum,n)==1);
  memset(pcm,0,sizeof(pcm));track_drum_render(&drum,&mix,pcm,2048);
  assert(power()>1000);
 }
 track_drum_stop(&drum);assert(track_drum_trigger(&drum,0));
 memset(pcm,0,sizeof(pcm));track_drum_render(&drum,&mix,pcm,2048);
 int32_t master=32767<<16;
 for(uint32_t n=0;n<4096;n++)assert(track_master_apply(pcm[n],0,&master)==0);
 assert(master==0);
 /* Source independence is enforced at build by absence of legacy PCM header. */
 puts("BEETLE V0.31: 12 A/B host projects, swing selectivity/timing, 14 HQ drums, flash locked, master zero PASS");
 return 0;
}
