/* SPDX-License-Identifier: GPL-3.0-only
 * Transactional V2 NOR writer. Caller owns all working RAM.
 * Never call in DMA, audio, USB interrupt, or while audio is running.
 */
#ifndef BEETLE_STORAGE_V2_WRITER_H
#define BEETLE_STORAGE_V2_WRITER_H
#include <stdint.h>
#include "beetle_storage_v2_reader.h"
#define BEETLE_V2_PAGE 256u
#define BEETLE_V2_SECTOR 4096u
#define BEETLE_V2_GUARD_SECTORS 4u
/* Beyond the four crash-recovery sectors, leave space for further project
 * journal commits. One 12 KiB project write may need four erased sectors.
 * This is a MINIMUM project-write allowance, not a guarantee that twelve
 * arbitrarily large project revisions can all fit in shared flash. */
#define BEETLE_V2_PROJECT_RESERVE_SECTORS 8u
/* Callbacks accept physical absolute flash addresses, 0 on success. */
typedef int (*beetle_v2_program)(void *ctx,uint32_t address,const uint8_t *src,uint32_t length);
typedef int (*beetle_v2_erase)(void *ctx,uint32_t sector_address);
struct beetle_v2_writer {
  struct beetle_v2_device device;
  beetle_v2_program program;
  beetle_v2_erase erase;
  void *context;
  /* protected records from both superblock generations; 0 means free */
  uint8_t domain[72];
  uint16_t occupied[72];
  uint8_t active_meta;
};
/* caller index and other_index must each be 4096 bytes; page 256 bytes.
 * Requires an already migrated, valid Storage V2 index (not legacy TRKJ).
 * -1 I/O or malformed; -2 STORAGE FULL; -3 unowned/corrupt; -4 verify.
 */
int beetle_v2_writer_open(struct beetle_v2_writer *w,beetle_v2_read read,
 beetle_v2_program program,beetle_v2_erase erase,void *ctx,
 uint8_t *index,uint8_t *other_index);
/* Data is an already compressed BPK1 project, or BEETLE DPCM4 sample bank.
 * No compression, malloc or audio decoding is performed by this module.
 */
int beetle_v2_write_project(struct beetle_v2_writer *w,uint8_t slot,
 const uint8_t *data,uint32_t length,uint8_t *index,uint8_t *other_index,
 uint8_t page[256]);
int beetle_v2_write_sample(struct beetle_v2_writer *w,uint8_t slot,
 const uint8_t *data,uint32_t length,uint8_t *index,uint8_t *other_index,
 uint8_t page[256]);
int beetle_v2_remove_project(struct beetle_v2_writer *w,uint8_t slot,
 uint8_t *index,uint8_t *other_index,uint8_t page[256]);
int beetle_v2_remove_sample(struct beetle_v2_writer *w,uint8_t slot,
 uint8_t *index,uint8_t *other_index,uint8_t page[256]);
/* Bounded streaming replacement, no complete-sample RAM allocation.
 * begin reserves every destination before writing a byte. The caller owns
 * 4068-byte chunk RAM, two 4096-byte index buffers, and this descriptor.
 * Feed is STRICTLY sequential; an interrupted transfer never commits.
 * Call only in main context while playback is paused and flash write permit
 * is held. Never run from USB/audio interrupts. */
struct beetle_v2_sample_upload {
 uint32_t destinations[128];
 uint32_t total,expected_crc,accumulator,received,generation;
 uint16_t chunks,filled,current,slot,index_pos;
 uint8_t active;
};
int beetle_v2_sample_upload_begin(struct beetle_v2_writer *w,
 struct beetle_v2_sample_upload *u,uint8_t slot,uint32_t total,
 uint32_t expected_crc,uint8_t *idx,uint8_t *other,uint8_t page[256]);
int beetle_v2_sample_upload_feed(struct beetle_v2_writer *w,
 struct beetle_v2_sample_upload *u,const uint8_t *data,uint16_t len,
 uint8_t *idx,uint8_t page[256],uint8_t chunk[4068]);
int beetle_v2_sample_upload_commit(struct beetle_v2_writer *w,
 struct beetle_v2_sample_upload *u,uint8_t *idx,uint8_t *other,
 uint8_t page[256],uint8_t chunk[4068]);
void beetle_v2_sample_upload_abort(struct beetle_v2_sample_upload *u);
/* Only erases fully unreferenced orphan sectors after checking BOTH indexes.
 * Never erases sector 0/1, firmware, OTA, or legacy marker sector.
 */
int beetle_v2_collect(struct beetle_v2_writer *w,uint8_t *index,uint8_t *other_index);
/* Read-only capacity estimate derived from BOTH verified metadata generations
 * and physically erased pages. Does not change NOR. Amounts are estimates:
 * fragmented pages and transaction metadata affect fit. Actual uploads are
 * authoritatively checked by sample_upload_begin before ANY NOR write. */
struct beetle_v2_sample_capacity {
 uint32_t current_sample_bytes,estimated_additional_bytes;
 uint16_t erased_free_sectors,sample_blank_pages;
};
int beetle_v2_get_sample_capacity(struct beetle_v2_writer *w,
 const uint8_t *newest_index,uint8_t page[256],
 struct beetle_v2_sample_capacity *out);
#endif
