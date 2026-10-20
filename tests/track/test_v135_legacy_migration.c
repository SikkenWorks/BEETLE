#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "track_project_store.h"
#include "track_master_fx.h"
static struct track_store_project a,b;
static struct track_engine engine;
static unsigned char payload[TRACK_STORE_V4_MAX];
int main(int argc,char **argv){
 assert(argc==2);
#ifdef MAKE_V21
 track_init(&engine); track_song_init(&a.song,&engine);track_mixer_init(&a.mixer);track_master_default(&a.master);
 a.bpm=120;a.slot=0;a.song.count=1;
 for(unsigned i=0;i<8;i++){a.instruments[i].volume=65;a.instruments[i].cutoff=7000;track_va_patch_default(&a.instruments[i].va);}
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&a.drum[i]);
 for(unsigned i=0;i<5;i++)a.punch_param[i]=50;
 a.song.patterns[0].ch[0][0].note=60; a.song.patterns[0].ch[0][0].effect=2;
 uint8_t k0=track_extra_key(0,0,0);
 assert(track_extra_add(&a.song,0,k0,0,TRACK_FX_VOLUME,60)==1);
 assert(track_extra_add(&a.song,0,k0,1,TRACK_FX_VOLUME,67)==1);
 uint8_t k1=track_extra_key(0,1,0);
 a.song.patterns[0].ch[0][1].note=62; a.song.patterns[0].ch[0][1].effect=3;
 assert(track_extra_add(&a.song,0,k1,0,TRACK_FX_VOLUME,74)==1);
 uint32_t n;assert(track_store_encode(&a,payload,sizeof(payload),&n)==0);assert(payload[4]==21);
 FILE *out=fopen(argv[1],"wb");assert(out&&fwrite(payload,1,n,out)==n);fclose(out);
 printf("GEN V21 %u bytes\n",n);
#else
 FILE *in=fopen(argv[1],"rb");assert(in);size_t n=fread(payload,1,sizeof(payload),in);fclose(in);assert(payload[4]==21);
 assert(track_store_decode(&b,payload,(uint32_t)n)==0);
 uint8_t k0=track_extra_key(0,0,0),k1=track_extra_key(0,1,0);
 assert(track_extra_find(&b.song,0,k0,0)!=NULL);
 assert(track_extra_find(&b.song,0,k0,1)!=NULL);
 assert(track_extra_find(&b.song,0,k1,0)!=NULL);
 assert(track_extra_find(&b.song,0,k1,1)==NULL);
 assert(track_extra_find(&b.song,0,k0,0)->value==60);
 assert(track_extra_find(&b.song,0,k0,1)->value==67);
 uint32_t m;assert(track_store_encode(&b,payload,sizeof(payload),&m)==0&&payload[4]==22);
 memset(&a,0,sizeof(a));assert(track_store_decode(&a,payload,m)==0);
 assert(track_extra_find(&a.song,0,k0,1)->value==67);
 assert(track_extra_find(&a.song,0,k1,0)->value==74);
 printf("PASS: authentic V21 data -> V22 preserves FX2/FX3 identities\n");
#endif
}
