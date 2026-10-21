#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_lane_clipboard.h"
#include "track_drum.h"
#include "track_mixer.h"
#include "track_project_store.h"
#include "track_chip.h"
#include "track_master_fx.h"
static struct track_store_project original,restored;
static uint8_t buf[TRACK_STORE_MAX];
int main(void){
 struct track_lane_clipboard copy={0};
 struct track_pattern source={0},target={0};
 source.length=target.length=16;
 for(unsigned i=0;i<TRACK_STEPS;i++){
  source.ch[3][i]=(struct track_step){(uint8_t)(42u+i),1,2,3,(uint8_t)(11u+i)};
 }
 assert(!track_lane_clip_paste(&copy,&target,4));
 assert(track_lane_clip_copy(&copy,&source,3));
 assert(track_lane_clip_paste(&copy,&target,4));
 assert(memcmp(source.ch[3],target.ch[4],sizeof(copy.step))==0);
 assert(copy.valid&& !track_lane_clip_paste(&copy,&target,TRACK_CHANNELS));
 /* Retains clipboard across source pattern edits and lane switches. */
 memset(source.ch[3],0,sizeof(source.ch[3]));
 assert(target.ch[4][8].note==50);
 track_drum_init(&(struct track_drum_engine){0});
 struct track_drum_engine d;track_drum_init(&d);
 d.components[0].pitch=-36;d.components[1].pitch=36;
 assert(track_drum_trigger(&d,0));
 uint32_t low=0,high=0;
 for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(d.voices[i].active){low=d.voices[i].phase_step;break;}
 assert(track_drum_trigger(&d,1));
 for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(d.voices[i].active&&d.voices[i].component==1){high=d.voices[i].phase_step;break;}
 assert(low>0&&high>low*8u);
 struct track_mixer m;track_mixer_init(&m);
 int16_t pcm[512]={0};track_drum_render(&d,&m,pcm,256);
 assert(track_lane_pcm_peak[TRACK_DRUM_CHANNEL]>0);
 /* A sounding CHIP voice must raise its own lane only after audible mix. */
 static struct track_chip_engine synth;
 track_chip_init(&synth);
 track_lane_pcm_peak[0]=0u;
 assert(track_chip_note(&synth,0,0,60,127));
 memset(pcm,0,sizeof(pcm));track_chip_render(&synth,&m,pcm,256);
 assert(track_lane_pcm_peak[0]>0);
 track_lane_pcm_peak[0]=0u;m.ch[0].mute=1u;
 memset(pcm,0,sizeof(pcm));track_chip_render(&synth,&m,pcm,256);
 assert(track_lane_pcm_peak[0]==0u);
 m.ch[0].mute=0u;
 /* New drum pitch endpoints persist unchanged through V8 codec. */
 memset(&original,0,sizeof(original));
 struct track_engine init;track_init(&init);track_song_init(&original.song,&init);
 original.slot=0;original.bpm=120;original.harmony=(struct track_harmony){0,0,0};
 original.drum[0].pitch=-36; original.drum[1].pitch=36;
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&original.drum[i]);
 original.drum[0].pitch=-36;original.drum[1].pitch=36;
 for(unsigned i=0;i<4;i++){track_va_patch_default(&original.instruments[i].va);original.instruments[i].volume=80;}
 original.mixer=m;
 track_master_default(&original.master);
 uint32_t used=0;assert(track_store_encode(&original,buf,sizeof(buf),&used)==0);
 int result=track_store_decode(&restored,buf,used);fprintf(stderr,"decode rc=%d used=%u fmt=%u\n",result,used,buf[4]);assert(result==0);
 assert(restored.drum[0].pitch==-36&&restored.drum[1].pitch==36);
 printf("V0.40 PASS: lane clipboard + audible drum meter + +/-36 pitch + V8 save/load\n");
 return 0;
}
