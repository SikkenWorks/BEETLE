/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE tracker: a held-SEL range starts at the row under the cursor
 * when SEL is pressed, never at the first K1 detent.
 */
#ifndef BEETLE_STEP_SELECTION_H
#define BEETLE_STEP_SELECTION_H
#include <stdint.h>
static inline void beetle_step_selection_arm(uint8_t *active,uint8_t *anchor,
                                            uint8_t *end,uint8_t cursor){
 *anchor=cursor;
 *end=cursor;
 *active=0u; /* SEL tap remains the original clear-step gesture. */
}
static inline void beetle_step_selection_extend(uint8_t *active,uint8_t *end,
                                               uint8_t cursor){
 *end=cursor;
 *active=1u;
}
#endif
