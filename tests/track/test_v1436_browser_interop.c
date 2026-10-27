/* Portable V26 browser exports must decode unchanged on native firmware. */
#include <assert.h>
#include <stdio.h>
#include "track_project_store.h"
#include "track_drum.h"
static struct track_store_project project;
static unsigned char data[TRACK_STORE_V4_MAX];
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);size_t n=fread(data,1,sizeof data,f);fclose(f);int rc=track_store_decode(&project,data,(uint32_t)n);fprintf(stderr,"V26 browser interop decode status %d size %zu\n",rc,n);assert(rc==0);assert(project.drum_note3_enabled==1);assert(project.song.patterns[0].note3[2]==26);assert(project.song.patterns[0].note3[5]==TRACK_NOTE_STOP);assert(project.song.patterns[0].ch[5][1].note==3);puts("PASS V1.4.36 V26 WebSimulator→native decoder compatibility");}
