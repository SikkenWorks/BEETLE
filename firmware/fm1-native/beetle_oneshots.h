/* BEETLE V1.4.0: project-owned user one-shots, 22.05 kHz 8-bit original drum µ-law.
 * No new flash partition, no runtime allocation, no blocking sample reads.
 * The sum of all user samples MUST fit the project journal's 12 KiB payload.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef BEETLE_ONESHOTS_H
#define BEETLE_ONESHOTS_H
#include <stdint.h>
#define BEETLE_ONESHOT_FIRST 14u
#define BEETLE_ONESHOT_NOTES 13u
#define BEETLE_ONESHOT_LAST (BEETLE_ONESHOT_FIRST+BEETLE_ONESHOT_NOTES)
#define BEETLE_ONESHOT_MAX_BYTES 6144u
#define BEETLE_ONESHOT_FIRST_BYTES 4608u
#define BEETLE_ONESHOT_SECOND_BYTES 1536u
struct beetle_oneshot_bank {
 uint16_t length[BEETLE_ONESHOT_NOTES];
 char name[BEETLE_ONESHOT_NOTES][8];
 uint8_t *first, *second; /* two disjoint preallocated RAM blocks */
};
static inline uint32_t beetle_oneshot_used(const struct beetle_oneshot_bank *b){
 uint32_t n=0u;if(!b)return 0u;
 for(uint8_t i=0u;i<BEETLE_ONESHOT_NOTES;i++){
  n+=b->length[i];if(n>BEETLE_ONESHOT_MAX_BYTES)return BEETLE_ONESHOT_MAX_BYTES+1u;
 }return n;
}
/* Explicit split-bank access keeps real USB ring and boot mailbox memory separate. */
static inline uint8_t beetle_oneshot_get(const struct beetle_oneshot_bank *b,uint32_t at){
 return (!b||at>=BEETLE_ONESHOT_MAX_BYTES)?0u:
  at<BEETLE_ONESHOT_FIRST_BYTES?(b->first?b->first[at]:0u):
  (b->second?b->second[at-BEETLE_ONESHOT_FIRST_BYTES]:0u);
}
static inline void beetle_oneshot_set(struct beetle_oneshot_bank *b,uint32_t at,uint8_t value){
 if(!b||at>=BEETLE_ONESHOT_MAX_BYTES)return;
 if(at<BEETLE_ONESHOT_FIRST_BYTES){if(b->first)b->first[at]=value;}
 else if(b->second)b->second[at-BEETLE_ONESHOT_FIRST_BYTES]=value;
}
#endif
