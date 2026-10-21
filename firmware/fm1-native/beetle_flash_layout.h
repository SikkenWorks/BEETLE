/* SPDX-License-Identifier: GPL-3.0-only
 * Standalone BEETLE V0.33 storage partition, JieLi FM1 1MiB NOR.
 * IMPORTANT: overlaps historical ETCH/Felucca USER DATA, not the application,
 * boot header or OTA. Manual consent erases legacy USER DATA in this range.
 * Never enable this partition in a dual-instrument ETCH/BEETLE build.
 */
#ifndef BEETLE_FLASH_LAYOUT_H
#define BEETLE_FLASH_LAYOUT_H
#include <stdint.h>
#include "track_journal.h"
#define BEETLE_FLASH_SECTOR 4096u
#define BEETLE_FLASH_BASE 0x00097000u
#define BEETLE_FLASH_END  0x000DF000u
#define BEETLE_FLASH_MARKER 0x000DF000u
#define BEETLE_FLASH_MARKER_END 0x000E0000u
#define BEETLE_FLASH_REQUIRED_BYTES TRACK_JOURNAL_PROVISION_BYTES
#define BEETLE_FLASH_PARTITION_APPROVED 1u /* standalone WITH manual destructive consent */
_Static_assert(TRACK_JOURNAL_SLOTS==12u,"12 project slots required");
_Static_assert(TRACK_JOURNAL_BANK_BYTES==3u*BEETLE_FLASH_SECTOR,"12KiB A/B banks required");
_Static_assert(TRACK_JOURNAL_PROVISION_BYTES==0x48000u,"288KiB physical storage required");
_Static_assert(BEETLE_FLASH_BASE+TRACK_JOURNAL_PROVISION_BYTES==BEETLE_FLASH_END,"bank area must end before marker");
_Static_assert(BEETLE_FLASH_MARKER_END==0xE0000u,"OTA boundary must not change");
_Static_assert(TRACK_STORE_V4_MAX+TRACK_JOURNAL_HEADER_BYTES<=TRACK_JOURNAL_BANK_BYTES,"V4 serializer exceeds flash bank");
int beetle_flash_map(uint8_t slot,uint8_t bank,uint32_t offset,uint32_t n,uint32_t *physical);
typedef int (*beetle_flash_read_fn)(void *,uint32_t,uint8_t *,uint32_t);
enum beetle_claim_result {BEETLE_CLAIM_FOREIGN=-3,BEETLE_CLAIM_IO=-2,BEETLE_CLAIM_INVALID=-1,BEETLE_CLAIM_BLANK=0,BEETLE_CLAIM_EXISTING=1};
int beetle_flash_claim_probe(beetle_flash_read_fn read,void *ctx);
void beetle_flash_make_marker(uint8_t out[32]);
/* Read-only marker recovery audit. 1=repairable candidate, 0=foreign/unsafe, -1=I/O.
 * A candidate alone NEVER authorizes writes: audit all A/B banks separately. */
int beetle_flash_marker_repairable(beetle_flash_read_fn read,void *ctx);
#endif
