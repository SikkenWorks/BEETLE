/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../../firmware/fm1-native/beetle_project_delete_ui.h"
#define S BEETLE_CONFIRM_SEL
#define E BEETLE_DELETE_EDIT
#define G BEETLE_DELETE_GLO
struct model {
 struct beetle_select_transaction boot,load;
 uint8_t page,choice,status,armed,delete_popup,created,loaded;
};
static void reset(struct model *m){*m=(struct model){0};m->page=29;m->status=1;m->armed=1;}
static void poll(struct model *m,uint32_t t,uint32_t held,uint32_t pressed,uint32_t released){
 if(m->page==29){
  if(beetle_boot_select_confirm(&m->boot,t,held,pressed,released)){
   if(m->choice==0)m->created++;
   else m->page=14;
  }
 }else if(m->page==14){
  if(beetle_delete_hotkey(14,m->status,held,pressed,m->armed)){
   m->delete_popup=1;m->page=39;beetle_select_reset(&m->load);
  }else if(beetle_load_select_confirm(&m->load,t,held,pressed,released)){
   m->loaded++;
  }
 }
}
static void boot_cases(void){
 struct model m;
 /* Plain SEL only, not until 180ms clear idle. */
 reset(&m);poll(&m,1,S,S,0);poll(&m,20,0,0,S);poll(&m,199,0,0,0);
 assert(!m.created);poll(&m,200,0,0,0);assert(m.created==1);
 /* Chord held: edit-first. */
 reset(&m);poll(&m,1,E,E,0);poll(&m,20,E|S,S,0);poll(&m,40,E,0,S);
 poll(&m,60,0,0,E);poll(&m,400,0,0,0);assert(!m.created);
 /* Chord held: sel-first. */
 reset(&m);poll(&m,1,S,S,0);poll(&m,20,S|E,E,0);poll(&m,40,E,0,S);
 poll(&m,60,0,0,E);poll(&m,400,0,0,0);assert(!m.created);
 /* Important missing case in V1.6.8: EDIT fully pressed+released
  * while SEL is down, BETWEEN two UI polls. No held EDIT ever seen. */
 reset(&m);poll(&m,1,S,S,0);poll(&m,20,S,E,E);poll(&m,40,0,0,S);
 poll(&m,300,0,0,0);assert(!m.created);
 /* Equally important: delayed EDIT edge on the SEL release poll. */
 reset(&m);poll(&m,1,S,S,0);poll(&m,20,0,E,S|E);
 poll(&m,300,0,0,0);assert(!m.created);
 /* Modifier debounced only AFTER SEL up, before settling completes. */
 reset(&m);poll(&m,1,S,S,0);poll(&m,20,0,0,S);
 poll(&m,90,0,E,E);poll(&m,300,0,0,0);assert(!m.created);
 /* LOAD choice cannot turn into NEW */
 reset(&m);m.choice=1;poll(&m,1,S,S,0);poll(&m,20,0,0,S);
 poll(&m,201,0,0,0);assert(m.page==14&&!m.created);
 /* A multi-button tap, even if EDIT didn't overlap SEL held. */
 reset(&m);poll(&m,1,S,S,0);poll(&m,20,0,0,S);poll(&m,30,G,G,0);
 poll(&m,50,0,0,G);poll(&m,300,0,0,0);assert(!m.created);
}
static void load_cases(void){
 struct model m;
 reset(&m);m.page=14;poll(&m,1,S,S,0);poll(&m,20,0,0,S);
 poll(&m,129,0,0,0);assert(!m.loaded);poll(&m,130,0,0,0);assert(m.loaded==1);
 reset(&m);m.page=14;poll(&m,1,S,S,0);poll(&m,20,S,E,E);
 poll(&m,40,0,0,S);poll(&m,200,0,0,0);assert(!m.loaded);
 reset(&m);m.page=14;poll(&m,1,E,E,0);poll(&m,20,E|S,S,0);
 assert(m.delete_popup && !m.loaded && !m.created);
 reset(&m);m.page=14;poll(&m,1,S,S,0);poll(&m,20,S|E,E,0);
 assert(m.delete_popup && !m.loaded && !m.created);
 /* Late modifier cancels a pending normal LOAD. */
 reset(&m);m.page=14;poll(&m,1,S,S,0);poll(&m,20,0,0,S);
 poll(&m,80,0,E,E);poll(&m,200,0,0,0);assert(!m.loaded);
}
int main(void){boot_cases();load_cases();puts("V1.6.9 selector/chord timing: PASS");return 0;}
