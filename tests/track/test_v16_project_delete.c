#include <assert.h>
#include <stdint.h>
#include "../../firmware/fm1-native/beetle_project_delete_ui.h"
int main(void){
 const uint32_t chord=BEETLE_DELETE_CHORD;
 assert(chord==(BEETLE_DELETE_EDIT|BEETLE_CONFIRM_SEL));
 assert(!beetle_delete_hotkey(14u,1u,chord,BEETLE_DELETE_EDIT,0u));
 assert(beetle_delete_hotkey(14u,1u,chord,BEETLE_DELETE_EDIT,1u));
 assert(beetle_delete_hotkey(14u,1u,chord,BEETLE_CONFIRM_SEL,1u));
 assert(!beetle_delete_hotkey(14u,2u,chord,chord,1u));
 assert(!beetle_delete_hotkey(0u,1u,chord,chord,1u));
 assert(!beetle_delete_hotkey(14u,1u,BEETLE_DELETE_GLO|BEETLE_DELETE_EDIT,0u,1u));
 assert(beetle_delete_chord_released(0u));
 assert(!beetle_delete_chord_released(BEETLE_CONFIRM_SEL));
 assert(!beetle_delete_chord_released(BEETLE_DELETE_EDIT));
 assert(!beetle_delete_chord_released(chord));
 return 0;
}
