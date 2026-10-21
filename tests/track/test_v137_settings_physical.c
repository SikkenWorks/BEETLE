/* Host regression for real FM1 button matrix semantics.
 * Edge queue delivered at 18ms intervals; NOT a claim of hardware testing. */
#include <assert.h>
#include <stdint.h>
#include "beetle_settings_nav.h"
int main(void){
 const uint32_t S=BEETLE_SETTINGS_SEL;
 struct beetle_settings_nav n={0};
 assert(!beetle_settings_enter(&n,1,0,0,0,0));
 assert(!beetle_settings_enter(&n,1,6,0,0,0));
 /* The normal rising edge enters the page before generic menu/track logic. */
 assert(beetle_settings_enter(&n,1,6,S,S,0));
 assert(!n.toggle_armed);
 assert(!beetle_settings_toggle(&n,S,0,0));
 /* Opening click release must not toggle USB mode. */
 assert(!beetle_settings_toggle(&n,0,0,S));assert(n.toggle_armed);
 assert(beetle_settings_toggle(&n,S,S,0));
 assert(!beetle_settings_toggle(&n,S,0,0));
 assert(!beetle_settings_toggle(&n,0,0,S));
 assert(beetle_settings_toggle(&n,S,S,0));
 /* Fast press+release queued together while still entering: no false toggle. */
 n=(struct beetle_settings_nav){0};
 assert(beetle_settings_enter(&n,1,6,0,S,S));
 assert(!beetle_settings_toggle(&n,0,0,S));
 /* Missed rising edge fallback: only a hold ON THE MENU can be completed. */
 n=(struct beetle_settings_nav){0};
 assert(!beetle_settings_enter(&n,1,6,S,0,0));
 assert(beetle_settings_enter(&n,1,6,0,0,S));
 assert(!beetle_settings_enter(&n,1,6,0,0,S));
 /* No accidental transition when another PROJECTS option is selected. */
 n=(struct beetle_settings_nav){0};
 assert(!beetle_settings_enter(&n,1,3,S,S,0));
 assert(!beetle_settings_enter(&n,1,3,0,0,S));
 /* Leaving project menu revokes any stale key latch. */
 assert(!beetle_settings_enter(&n,0,6,S,0,0));
 assert(!beetle_settings_enter(&n,1,6,0,0,S));
 /* Neither HOME nor SEQ have the SEL key bit. */
 assert(!beetle_settings_enter(&n,1,6,(1u<<8)|(1u<<11),(1u<<8)|(1u<<11),0));
 return 0;
}
