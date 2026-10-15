/* SPDX-License-Identifier: GPL-3.0-only
 * Regression: a project confirmation SEL must never clear the tracker row.
 */
#include <assert.h>
#include <stdint.h>
#include "beetle_input_safety.h"
int main(void){
 struct beetle_load_input_gate g={0};
 const uint32_t SEL=BEETLE_SELECT_BUTTON;
 assert(beetle_step_delete_eligible(0,0,&g,SEL)); /* genuine Tracker press */
 assert(!beetle_step_delete_eligible(1,0,&g,SEL)); /* PROJECTS -> tracker */
 assert(!beetle_step_delete_eligible(18,0,&g,SEL)); /* LOADING/VERIFYING */
 assert(!beetle_step_delete_eligible(2,0,&g,SEL)); /* SAVE name -> tracker */
 assert(!beetle_step_delete_eligible(0,18,&g,SEL)); /* save began in tracker */
 assert(!beetle_step_delete_eligible(0,1,&g,SEL)); /* screen left tracker */
 assert(!beetle_step_delete_eligible(0,0,&g,0));
 /* Press/release/queued press during synchronous flash I/O. */
 beetle_load_input_arm(&g,100);
 uint32_t p=SEL,r=0;
 beetle_load_input_filter(&g,200,SEL,&p,&r);
 assert(p==0&&g.active&& !beetle_step_delete_eligible(0,0,&g,SEL));
 p=SEL;r=SEL;
 beetle_load_input_filter(&g,300,0,&p,&r);
 assert(p==0 && r==0 && g.active);
 p=SEL;r=0;
 beetle_load_input_filter(&g,500,0,&p,&r);
 assert(p==0&&g.active);
 p=SEL;r=0;
 beetle_load_input_filter(&g,621,0,&p,&r);
 assert(!g.active && !p);  /* first unblocked poll also consumes */
 p=SEL;
 beetle_load_input_filter(&g,640,SEL,&p,&r);
 assert(beetle_step_delete_eligible(0,0,&g,p));
 /* Saving must also fence a confirmed selection during an interrupted write. */
 beetle_load_input_arm(&g,1500);
 assert(!beetle_step_delete_eligible(0,0,&g,SEL));
 p=SEL;r=0;
 beetle_load_input_filter(&g,1518,0,&p,&r);
 assert(!p);
 return 0;
}
