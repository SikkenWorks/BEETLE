/* V0.35 six lanes, V5 sparse storage, legacy four-lane migration, 12 A/B journals. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "track_core.h"
#include "track_song.h"
#include "track_chip.h"
#include "track_drum.h"
#include "track_mixer.h"
#include "track_swing.h"
#include "track_master_fx.h"
#include "track_journal.h"
static uint8_t disks[12][2][TRACK_JOURNAL_BANK_BYTES],scratch[TRACK_STORE_MAX];
static struct track_store_project p,q;
static int failafter=-1,erase_count=0,write_count=0;
static int rd(void*x,uint8_t sl,uint8_t b,uint32_t off,uint8_t*dst,uint32_t n){(void)x;if(sl>=12||b>=2||off>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-off)return -1;memcpy(dst,disks[sl][b]+off,n);return 0;}
static int er(void*x,uint8_t sl,uint8_t b){(void)x;if(sl>=12||b>=2)return -1;memset(disks[sl][b],255,TRACK_JOURNAL_BANK_BYTES);erase_count++;return 0;}
static int wr(void*x,uint8_t sl,uint8_t b,uint32_t off,const uint8_t*src,uint32_t n){(void)x;if(sl>=12||b>=2||off>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-off||(off&255u)+n>256)return -1;if(failafter==0)return -1;if(failafter>0)failafter--;for(uint32_t i=0;i<n;i++){if((disks[sl][b][off+i]&src[i])!=src[i])return -1;disks[sl][b][off+i]&=src[i];}write_count++;return 0;}
static const struct track_journal_io io={0,rd,er,wr};
static void make(uint8_t slot,uint16_t bpm){
 struct track_engine e;track_init(&e);memset(&p,0,sizeof(p));track_song_init(&p.song,&e);track_mixer_init(&p.mixer);
 track_master_default(&p.master);for(unsigned d=0;d<TRACK_DRUM_COUNT;d++)track_drum_timbre_default(&p.drum[d]);
 p.slot=slot;p.bpm=bpm;p.song.count=64;p.song.enabled=1;p.song.loop=1;memcpy(p.title,"SIX TRACKER",11);
 for(unsigned x=0;x<64;x++){p.song.chain[x].pattern=(uint8_t)(x%48);p.song.chain[x].repeats=(uint8_t)(1+x%16);}
 for(unsigned x=0;x<4;x++){p.instruments[x].type=x;p.instruments[x].attack=x;p.instruments[x].decay=11+x;p.instruments[x].sustain=99-x;p.instruments[x].release=17+x;p.instruments[x].duty=50;p.instruments[x].cutoff=7000+x*100;p.instruments[x].volume=77+x;track_va_patch_default(&p.instruments[x].va);p.instruments[x].va.coarse=(int8_t)x;}
 for(unsigned x=0;x<6;x++){p.mixer.ch[x].volume=51+x;p.mixer.ch[x].pan=(int8_t)(x*11-33);p.mixer.ch[x].mute=(x==3);p.mixer.ch[x].solo=(x==4);p.mixer.ch[x].swing=x&1;}
 for(unsigned pat=0;pat<48;pat++){
  for(unsigned ch=0;ch<6;ch++)for(unsigned step=0;step<16;step++)if(((pat*3+ch*5+step)%7)==0){
   struct track_step *v=&p.song.patterns[pat].ch[ch][step];v->note=ch==5?6:(uint8_t)(48+(ch*3+step)%38);v->instrument=ch&3;v->chord=ch==5?2:ch%12;v->effect=TRACK_FX_VOLUME;v->value=(uint8_t)((step*6)%101);
  }
  p.song.clips[pat].count=2;p.song.clips[pat].data[0]=(struct track_song_event){0,3,1};p.song.clips[pat].data[1]=(struct track_song_event){15,3,0};
 }
}
static void check_project(void){
 assert(q.slot==p.slot && q.bpm==p.bpm && q.song.count==p.song.count);
 for(unsigned ch=0;ch<6;ch++)assert(!memcmp(&q.mixer.ch[ch],&p.mixer.ch[ch],sizeof(p.mixer.ch[0])));
 for(unsigned ins=0;ins<4;ins++)assert(!memcmp(&q.instruments[ins],&p.instruments[ins],sizeof(p.instruments[0])));
 for(unsigned pat=0;pat<48;pat++){
  assert(q.song.patterns[pat].length==p.song.patterns[pat].length);
  for(unsigned ch=0;ch<6;ch++)for(unsigned st=0;st<16;st++)assert(!memcmp(&q.song.patterns[pat].ch[ch][st],&p.song.patterns[pat].ch[ch][st],sizeof(struct track_step)));
  assert(q.song.clips[pat].count==p.song.clips[pat].count);
 }
}
int main(int argc,char**argv){
 assert(TRACK_CHANNELS==6 && TRACK_SYNTH_CHANNELS==5 && TRACK_DRUM_CHANNEL==5 && TRACK_CHIP_VOICES==8);
 struct track_engine e;track_init(&e);assert(e.transport.edit_channel==0&&e.active.ch[5][0].instrument==3);
 for(unsigned i=0;i<6;i++){track_set_channel(&e,i);assert(e.transport.edit_channel==i);}
 track_set_channel(&e,4);e.transport.edit_step=15;e.transport.edit_column=0;
 assert(track_enter_note(&e,61)==0 && e.transport.edit_step==0 && e.active.ch[4][15].note==61);
 track_set_channel(&e,5);assert(track_enter_note(&e,3)==0 && e.transport.edit_step==1);
 struct track_mixer m;track_mixer_init(&m);for(unsigned c=0;c<6;c++){m.row=c;m.col=4;assert(track_mixer_change(&m,1));}assert(track_swing_mask(&m,3)==0x3f && track_swing_mask(&m,2)==0);
 struct track_chip_engine synth;track_chip_init(&synth);track_chip_set_mono(&synth,0);
 for(unsigned ch=0;ch<5;ch++)assert(track_chip_note(&synth,ch,(uint8_t)(ch%4),60+ch,94));assert(synth.voice_count==5);
 for(unsigned j=5;j<14;j++)assert(track_chip_note(&synth,(uint8_t)(j%5),(uint8_t)(j%4),60+j,94));assert(synth.voice_count==8 && synth.steal_count>0);
 assert(track_chip_health(&synth));track_chip_stop_channel(&synth,4);
 memset(disks,255,sizeof(disks));
 for(unsigned slot=0;slot<12;slot++){
  make(slot,120+slot);uint32_t n=0;assert(track_store_encode(&p,scratch,sizeof(scratch),&n)==0);assert(n<=TRACK_JOURNAL_BANK_BYTES-TRACK_JOURNAL_HEADER_BYTES);
  assert(track_journal_save(&io,slot,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
  assert(track_journal_load(&io,slot,&q,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);check_project();
 }
 make(6,126);int writes=write_count,erases=erase_count;assert(track_journal_save(&io,6,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);assert(write_count==writes&&erase_count==erases);
 make(6,222);failafter=3;assert(track_journal_save(&io,6,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_IO);failafter=-1;
 assert(track_journal_load(&io,6,&q,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK&&q.bpm==126);
 assert(track_journal_save(&io,6,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);assert(track_journal_load(&io,6,&q,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);check_project();
 /* All 48 patterns at maximal entropy MUST be rejected BEFORE flash erase. */
 make(2,127);
 for(unsigned pat=0;pat<48;pat++)for(unsigned ch=0;ch<6;ch++)for(unsigned st=0;st<16;st++){
  struct track_step *v=&p.song.patterns[pat].ch[ch][st];v->note=ch==5?(uint8_t)(st%14):(uint8_t)(ch*17+st);v->instrument=st%4;v->chord=ch==5?4:st%12;v->effect=(uint8_t)(st%11);v->value=st*5;
 }
 writes=write_count;erases=erase_count;assert(track_journal_save(&io,2,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_CAPACITY);assert(writes==write_count&&erases==erase_count);
 if(argc==2){
  FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);assert(n>0&&n<=TRACK_STORE_MAX);assert(fread(scratch,1,n,f)==(size_t)n);fclose(f);
  assert(track_store_decode(&q,scratch,n)==0);assert(q.slot==11&&q.bpm==134&&q.song.patterns[47].ch[5][12].note==4);
  assert(q.song.patterns[47].ch[3][12].note==TRACK_NOTE_EMPTY);assert(q.mixer.ch[2].swing==50);
 }
 if(argc>=3){
  FILE*f=fopen(argv[2],"rb");assert(f);fseek(f,0,SEEK_END);long len=ftell(f);rewind(f);assert(len>0&&len<=TRACK_STORE_MAX);
  assert(fread(scratch,1,len,f)==(size_t)len);fclose(f);assert(track_store_decode(&q,scratch,len)==0);
  assert(q.bpm==143&&q.slot==8&&q.song.patterns[23].ch[5][12].note==4);
  assert(q.song.patterns[23].ch[2][5].effect==TRACK_FX_VOLUME&&q.song.patterns[23].ch[2][5].value==67);
  assert(q.song.patterns[23].ch[3][12].note==TRACK_NOTE_EMPTY&&q.mixer.ch[5].swing==50&&q.mixer.ch[5].volume==92);
 }
 puts("PASS V0.35 six lanes, 8 synth voices/steal, swing six, 12 CRC A/B projects, all parameters, torn write rollback, capacity lock, legacy V3 migration");return 0;
}
