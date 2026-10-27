/* BEETLE V1.7.12: LOAD/DISCARD is not NEW PROJECT.
 * Pure no-I/O state helpers, shared by firmware and host regression.
 * No stored project format changes and no DMA/flash operations. */
#ifndef BEETLE_LOAD_NAVIGATION_H
#define BEETLE_LOAD_NAVIGATION_H
#include <stdint.h>
struct beetle_load_navigation { uint8_t original_slot, discard_approved; };
static inline void beetle_load_navigation_begin(struct beetle_load_navigation *n,uint8_t source_slot){
 n->original_slot=source_slot;n->discard_approved=0u;
}
static inline void beetle_load_navigation_approve(struct beetle_load_navigation *n){
 n->discard_approved=1u;
}
static inline uint8_t beetle_load_navigation_needs_discard(const struct beetle_load_navigation *n,uint8_t dirty){
 return (uint8_t)(dirty && !n->discard_approved);
}
static inline uint8_t beetle_load_navigation_cancel(struct beetle_load_navigation *n){
 uint8_t original=n->original_slot;n->discard_approved=0u;return original;
}
static inline void beetle_load_navigation_end(struct beetle_load_navigation *n){
 n->original_slot=0u;n->discard_approved=0u;
}
#endif
