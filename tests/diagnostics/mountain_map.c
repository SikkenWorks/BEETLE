#include <stdio.h>
#include <stdint.h>
#include "track_project_store.h"
static struct track_store_project p;
static uint8_t buf[TRACK_STORE_MAX];
int main(int argc,char**argv){FILE*f=fopen(argv[1],"rb");int n=fread(buf,1,sizeof buf,f);fclose(f);if(track_store_decode(&p,buf,n)){puts("BAD");return 1;}
printf("SONG COUNT %u BPM %u\n",p.song.count,p.bpm);
for(int i=0;i<p.song.count;i++){struct track_song_entry *e=&p.song.chain[i];printf("POS %2u REPEAT %u: ",i+1,e->repeats);for(int c=0;c<TRACK_CHANNELS;c++){int pat=e->lane[c];int count=0, chords=0; if(pat<TRACK_PATTERNS)for(int k=0;k<TRACK_STEPS;k++){struct track_step *s=&p.song.patterns[pat].ch[c][k];if(s->note<128){count++;if(s->chord)chords++;}}printf("CH%d P%02u (%d notes/%d chords) ",c+1,pat+1,count,chords);}puts("");}
for(int k=0;k<8;k++)printf("INST%d = TYPE %d\n",k+1,p.instruments[k].type);
return 0;}
