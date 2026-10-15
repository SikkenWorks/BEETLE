#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "track_drum.h"
#include "track_mixer.h"
static void le16(FILE*f,uint16_t v){fputc(v&255,f);fputc(v>>8,f);}
static void le32(FILE*f,uint32_t v){le16(f,(uint16_t)v);le16(f,(uint16_t)(v>>16));}
static int wav(const char*path,const int16_t*data,uint32_t frames){FILE*f=fopen(path,"wb");if(!f)return 1;fwrite("RIFF",1,4,f);le32(f,36u+frames*4u);fwrite("WAVEfmt ",1,8,f);le32(f,16);le16(f,1);le16(f,2);le32(f,44100);le32(f,44100*4);le16(f,4);le16(f,16);fwrite("data",1,4,f);le32(f,frames*4);fwrite(data,4,frames,f);fclose(f);return 0;}
int main(int argc,char**argv){if(argc!=2)return 2;char path[512];struct track_mixer m;track_mixer_init(&m);const int N=66000;int16_t*buf=calloc(N*2,sizeof(int16_t));if(!buf)return 1;
for(int drum=0;drum<TRACK_DRUM_COUNT;drum++){memset(buf,0,N*4);struct track_drum_engine d;track_drum_init(&d);track_drum_trigger(&d,drum);track_drum_render(&d,&m,buf,N);snprintf(path,sizeof(path),"%s/%02d-%s.wav",argv[1],drum,track_drum_name(drum));if(wav(path,buf,N))return 3;}
/* A minimal musical groove. Sixteen steps, 110 BPM; render each chunk using
 * the SAME engine and accurate note-on timing, for summing & headroom tests. */
const int bpm=110,barframes=(int)((44100.0*60.0*4)/bpm);const int steps=16;int frames=barframes*2;if(frames>N*3){}int16_t*mix=calloc((size_t)frames*2,sizeof(int16_t));struct track_drum_engine d;track_drum_init(&d);int off=0;
for(int step=0;step<steps*2;step++){int start=(step*barframes)/16,stop=((step+1)*barframes)/16;int phase=step%16;if(phase==0||phase==8)track_drum_trigger(&d,0);if(phase==4||phase==12)track_drum_trigger(&d,1);if((phase%2)==0)track_drum_trigger(&d,2);if(phase==10)track_drum_trigger(&d,3);if(phase==14)track_drum_trigger(&d,7);track_drum_render(&d,&m,mix+2*start,stop-start);off=stop;}snprintf(path,sizeof(path),"%s/BEETLE-V0.43-Factory-Kit-Groove.wav",argv[1]);if(wav(path,mix,off))return 4;free(buf);free(mix);puts("WAV PREVIEW GENERATED");return 0;}
