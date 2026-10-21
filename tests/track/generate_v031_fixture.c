/* Compile against original V0.31 source ONLY to freeze a genuine V3 fixture. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_project_store.h"
#include "track_core.h"
#include "track_song.h"
#include "track_mixer.h"
static struct track_store_project p;
static uint8_t buf[TRACK_STORE_MAX];
int main(int argc,char**argv){
 if(argc!=2)return 2;
 struct track_engine e;track_init(&e);track_song_init(&p.song,&e);track_mixer_init(&p.mixer);
 p.slot=11;p.bpm=134;p.mixer.ch[2].swing=1;
 memcpy(p.title,"LEGACY 031",10);
 p.song.patterns[47].ch[0][5].note=72;
 p.song.patterns[47].ch[0][5].chord=7;
 p.song.patterns[47].ch[1][10].effect=10;
 p.song.patterns[47].ch[1][10].value=31;
 p.song.patterns[47].ch[3][12].note=4;
 for(int k=0;k<4;k++){
  p.instruments[k].type=k;p.instruments[k].volume=64;p.instruments[k].duty=50;
  p.instruments[k].cutoff=7000;track_va_patch_default(&p.instruments[k].va);
 }
 p.instruments[0].va.coarse=9;p.instruments[0].va.fine=-25;p.instruments[0].va.pulse_width=32;
 unsigned n=0;
 if(track_store_encode(&p,buf,sizeof(buf),&n)!=0)return 4;
 FILE *f=fopen(argv[1],"wb");if(!f)return 5;
 fwrite(buf,1,n,f);fclose(f);
 printf("V0.31 V3 fixture %u bytes\n",n);return 0;
}
