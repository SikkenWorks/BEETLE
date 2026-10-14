/* V0.41: Factory kit, retained Punch-In ALGORITHM per project, reverse timeline.
 * Host-only tests, never indicate hardware safety. */
#include "track_chip.h"
#include "track_drum.h"
#include "track_project_store.h"
#include "track_master_fx.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct track_store_project project,loaded;
static uint8_t bytes[TRACK_STORE_MAX];
static struct track_chip_engine chip;
static int16_t pcm[8000*2];
int main(void){
 struct track_drum_engine kit;track_drum_init(&kit);
 assert(kit.components[0].eq_low>=70);
 assert(kit.components[1].eq_high>=65);
 assert(kit.components[2].release<kit.components[3].release);
 assert(kit.components[4].pitch!=kit.components[6].pitch);
 assert(kit.components[7].release>kit.components[2].release);
 struct track_mixer mix;track_mixer_init(&mix);
 assert(mix.ch[TRACK_DRUM_CHANNEL].volume==100);
 assert(mix.ch[0].volume==80);
 assert(track_drum_trigger(&kit,0));track_drum_render(&kit,&mix,pcm,512);
 unsigned nonzero=0;for(unsigned i=0;i<1024;i++)nonzero|=pcm[i]!=0;
 assert(nonzero);
 struct track_engine init;track_init(&init);memset(&project,0,sizeof(project));
 track_song_init(&project.song,&init);project.slot=3;project.bpm=120;
 for(unsigned c=0;c<4;c++){
  track_va_patch_default(&project.instruments[c].va);
  project.instruments[c].volume=80;
 }
 for(unsigned c=0;c<TRACK_DRUM_COUNT;c++)project.drum[c]=kit.components[c];
 project.mixer=mix;track_master_default(&project.master);
 const uint8_t params[5]={19,88,0,71,43};
 memcpy(project.punch_param,params,sizeof(params));
 uint32_t used=0;assert(track_store_encode(&project,bytes,sizeof(bytes),&used)==0);
 assert(bytes[4]==8u);
 assert(!track_store_decode(&loaded,bytes,used));
 assert(!memcmp(params,loaded.punch_param,5));
 assert(!memcmp(project.drum,loaded.drum,sizeof(project.drum)));
 /* Editing parameters must not be changed by entering or leaving FX. */
 track_chip_init(&chip);memcpy(chip.punch_param,params,5);
 for(unsigned i=0;i<8000;i++){int32_t t=(int32_t)i-4000;pcm[2*i]=(int16_t)t;pcm[2*i+1]=(int16_t)-t;}
 track_chip_process_punch(&chip,0,pcm,8000);
 for(unsigned i=0;i<8000;i++){pcm[2*i]=11000;pcm[2*i+1]=11000;}
 track_chip_process_punch(&chip,2u,pcm,6000);
 assert(chip.punch_param[1]==88);
 unsigned fall=0,rise=0;
 for(unsigned i=400;i<980;i++){
  if(pcm[2*i]<pcm[2*i-2])fall++;
  if(pcm[2*i]>pcm[2*i-2])rise++;
 }
 assert(fall>250&&rise<30); /* Actual backwards PCM, not distortion. */
 track_chip_process_punch(&chip,0,pcm,100);
 assert(chip.punch_param[1]==88);
 track_chip_recover(&chip);assert(chip.punch_param[1]==88);
 /* Real V7 fixture built by the unmodified V0.40 serializer. */
 FILE *f=fopen("tests/track/fixtures/beetle_v040_v7_original.trk","rb");assert(f);
 size_t v7size=fread(bytes,1,sizeof(bytes),f);fclose(f);
 assert(v7size>100&&bytes[4]==7u);
 assert(track_store_decode(&loaded,bytes,(uint32_t)v7size)==0);
 assert(loaded.bpm==135u&&loaded.slot==2u);
 assert(loaded.song.patterns[0].ch[0][0].note==64u);
 assert(loaded.song.patterns[0].ch[4][8].note==72u);
 assert(loaded.song.patterns[0].ch[TRACK_DRUM_CHANNEL][4].note==1u);
 assert(loaded.mixer.ch[1].swing==78u);
 assert(loaded.drum[0].eq_low==79u);
 assert(loaded.punch_param[0]==50u&&loaded.punch_param[2]==0u);
 puts("BEETLE V0.41 PASS: factory kit; drum gain; V8 save/load; retained ALGORITHM; reversed PCM; V0.40 V7 fixture; recover");
 return 0;
}
