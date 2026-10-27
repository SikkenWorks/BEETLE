#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../../firmware/fm1-native/beetle_load_navigation.h"

int main(void){
 struct beetle_load_navigation nav={0};
 /* Dirty project in slot 4: opening picker must not authorize a load until
    the user explicitly confirms DISCARD, and must never reset the song. */
 beetle_load_navigation_begin(&nav,3u);
 assert(beetle_load_navigation_needs_discard(&nav,1u));
 assert(!beetle_load_navigation_needs_discard(&nav,0u));
 beetle_load_navigation_approve(&nav);
 assert(!beetle_load_navigation_needs_discard(&nav,1u));
 /* Browsing another project only affects the picker cursor; cancelling
    restores the original SAVE target and revokes DISCARD permission. */
 assert(beetle_load_navigation_cancel(&nav)==3u);
 assert(beetle_load_navigation_needs_discard(&nav,1u));
 beetle_load_navigation_begin(&nav,8u);
 beetle_load_navigation_approve(&nav);
 assert(!beetle_load_navigation_needs_discard(&nav,1u));
 beetle_load_navigation_end(&nav);
 assert(beetle_load_navigation_needs_discard(&nav,1u));
 puts("PASS: discard-for-load permission, cancellable browser, original project slot, successful load reset");
 return 0;
}
