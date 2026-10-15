/* Confirms the actual FM1 preprocessor configuration hard-locks writes. */
#include <stdio.h>
#include <string.h>
#include "track_journal.h"
static struct track_store_project fake_project;
static uint8_t scratch[TRACK_STORE_MAX];
int main(void){
 memset(&fake_project,0,sizeof(fake_project));
 int r=track_journal_save(NULL,0u,&fake_project,scratch,sizeof(scratch),NULL);
 if(r!=TRACK_JOURNAL_HARDWARE_LOCKED){printf("FAIL firmware write lock: %d\n",r);return 1;}
 printf("PASS target always returns TRACK_JOURNAL_HARDWARE_LOCKED (%d)\n",r);return 0;
}
