/* V0.64: groove-only tools preserve musical data, respect pattern length. */
#include "beetle_groove_tools.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 struct track_pattern p={0};p.length=8;
 for(unsigned c=0;c<TRACK_CHANNELS;c++)for(unsigned i=0;i<TRACK_STEPS;i++){
  struct track_step *s=&p.ch[c][i];s->note=(uint8_t)(60+i);s->instrument=2;
  s->chord=3;s->effect=5;s->value=77;s->dynamics=0xAC;s->groove=0;
 }
 struct beetle_groove_clip g={0};
 struct track_step before=p.ch[0][2];
 track_groove_set_micro(&p.ch[0][2],7);
 track_groove_set_ratchet(&p.ch[0][2],4);
 track_groove_set_probability(&p.ch[0][2],2);
 beetle_groove_copy(&g,&p.ch[0][2]);assert(g.valid&&g.value==95);
 assert(beetle_groove_paste(&g,&p.ch[1][6])==1);
 assert(p.ch[1][6].groove==95&&p.ch[1][6].note==66 &&p.ch[1][6].dynamics==0xAC);
 assert(beetle_groove_paste(&g,&p.ch[1][6])==0);
 assert(beetle_groove_fill(&p,0,g.value)==1);
 for(unsigned i=0;i<8;i++)assert(p.ch[0][i].groove==95);
 for(unsigned i=8;i<16;i++)assert(p.ch[0][i].groove==0);
 assert(beetle_groove_fill(&p,0,g.value)==0);
 assert(beetle_groove_clear(&p,0)==1);
 assert(beetle_groove_clear(&p,0)==0);
 for(unsigned i=0;i<16;i++){
  assert(p.ch[0][i].note==60+i);
  assert(p.ch[0][i].dynamics==0xAC);
  assert(p.ch[0][i].effect==5);
  assert(p.ch[0][i].value==77);
 }
 assert(before.note==p.ch[0][2].note);
 assert(beetle_groove_fill(&p,6,79)==0);
 g.valid=0;assert(beetle_groove_paste(&g,&p.ch[1][1])==0);
 p.length=1;assert(beetle_groove_fill(&p,5,31)==1);
 assert(p.ch[5][0].groove==31&&p.ch[5][1].groove==0);
 printf("PASS V0.64 bounded groove COPY PASTE FILL CLEAR, other step fields untouched, 5-byte step\n");
 assert(sizeof(struct track_step)==5);
 return 0;
}
